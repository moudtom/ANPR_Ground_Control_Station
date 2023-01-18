#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <opencv2/opencv.hpp>
#include <QFileDialog>
#include <QRegExp>
#include <QMessageBox>
#include <QTimer>
#include "utilities.h"
#include <qlistwidget.h>
#include <QFile>
#include <fstream>
#include <sstream>
#include <iostream>
#include <QTextStream>
#include <QTextCodec>
using namespace cv;
using namespace dnn;
using namespace std;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      TBapi (nullptr)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tmrTimer = new QTimer(this);
    tmrTimer->start(33);
    // data configuration
    connect(ui->actionconfiguration, SIGNAL(triggered(bool)),this, SLOT(data_config()));

    // connect the signals and slots
    connect(ui->actionExit, SIGNAL(triggered(bool)), QApplication::instance(), SLOT(quit()));
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(openCamera()));

    //tcp  ip
    _server54000.myFrame.mbYolo = false;
    //_yolothread.myFrameYolo.bYolo=false;

    // take photo
    taking_photo = false;
    Auto_taking_photo = false;

    // Initial Yolo
    obj_names = objects_names_from_file(names_file);
    detector = new Detector(cfg_file ,weights_file);
    //Initial Tessaract
    TBapi = new tesseract::TessBaseAPI();
    TBapi->Init("C:/Program Files/Tesseract-OCR/tessdata", "tha");
    TBapi->SetPageSegMode(tesseract::PSM_AUTO_ONLY);

    // load color list
    myColorList = load_colorlist();

    // load blacklist
    myBlacklist = load_backlist();

    // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel(mainStatusBar);
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("ANPR Air Task Force 9 Program is Ready");

}

MainWindow::~MainWindow()
{
    _server54000.terminate();
    // Destroy used object and release memory
    if(TBapi != nullptr) {
        TBapi->End();
        delete TBapi;
    }
    delete ui;

}

vector<string> MainWindow::objects_names_from_file(const string filename)
{
    std::ifstream file(filename);
    std::vector<std::string> file_lines;
    if (!file.is_open()) return file_lines;
    for (std::string line; file >> line;) file_lines.push_back(line);
    std::cout << "object names loaded \n";
    return file_lines;
}

void MainWindow::openCamera()
{
    if (_server54000.bConnect) {
        _server54000.bConnect = false;
    }
    else {
        _server54000.setPort(54000,2000000);
        _server54000.ip_address ="127.0.0.1";
        //_server54000.ip_address ="192.168.1.254";
        _server54000.bConnect = true;
        _server54000.start();
        mainStatusLabel->setText("ANPR Ground Control Station is Connected");
    }
    connect(tmrTimer,SIGNAL(timeout()),this,SLOT(RunGui()));
    connect(this, &MainWindow::photoTaken, this, &MainWindow::appendSavedPhoto);
    //cap = VideoCapture(0,CAP_DSHOW);

}

void MainWindow::detectCarColor(cv::Mat inputImg)
{
    //if (inputImg.empty()) return "";

    cv::GaussianBlur(inputImg, inputImg, cv::Size(5, 5), 0, 0);
    myDetectColor.findHistrogram(inputImg);
    myFrame.imgColor = myDetectColor.color_rgb;
    myCar.colorHSV = myDetectColor.color_HSV;
    int _vh = (int)myCar.colorHSV.val[0];
    int _vs = (int)myCar.colorHSV.val[1];
    int _vv = (int)myCar.colorHSV.val[2];
    //qDebug()<<myColorList.size();
    //qDebug()<< _vh << "," << _vs << "," <<_vv;

    for (int i = 1; i < myColorList.size(); i++) {

        if (_vh > myColorList[i].hue_min && _vh < myColorList[i].hue_max &&
                _vs > myColorList[i].sat_min && _vs < myColorList[i].sat_max &&
                _vv > myColorList[i].val_min && _vv < myColorList[i].val_max)
        {
            myCar.str_color = myColorList[i].name;
            //qDebug()<<myCar.str_color.c_str();
            break;
        }
    }

    //cv::imshow("MY_WINDOW_COLOR",myFrame.imgColor);
}

bool MainWindow::detectCarPlate(cv::Mat &inputImg)
{
    std::vector<cv::Point> outPoint;

    if (find_large_contour1(inputImg, outPoint))
    {
        cv::Point2f srcPoints[] = {
            outPoint[0],
            outPoint[1],
            outPoint[2],
            outPoint[3],
        };

        cv::Size sizeOfPlate(plate_w,plate_h);
        cv::Mat temp_output(sizeOfPlate, CV_8UC3, cv::Scalar(0, 0, 0));
        
        cv::Point2f dstPoints[] = {
			cv::Point(0,0),
			cv::Point(temp_output.cols, 0),
			cv::Point(temp_output.cols, temp_output.rows),
			cv::Point(0, temp_output.rows),
		};

        cv::Mat myMatrix = cv::getPerspectiveTransform(srcPoints, dstPoints);
        cv::warpPerspective(inputImg, temp_output, myMatrix, temp_output.size());

        QImage framePlate2(
                    temp_output.data,
                    temp_output.cols,
                    temp_output.rows,
                    temp_output.step,
                    QImage::Format_RGB888);
        QPixmap imagePlate2 = QPixmap::fromImage(framePlate2);
        //ui->ShowImage_Plate_2->setPixmap(imagePlate2);

        //if(tempImg.rows > 85 && tempImg.cols > 180){
        temp_output.copyTo(myFrame.imgPlate);
        //}
        //if(tempImg.rows > 100 && tempImg.cols > 226)
        //tempImg (cv::Rect(0, 0, 226, 100)).copyTo(myFrame.imgPlate);

        return true;
    }
    else {
        myFrame.imgPlate = cv::Scalar(0, 0, 0);
        return false;
    }

}

void MainWindow::saveCarData(Car _t_car)
{
    //std::ofstream  file(saveDirectory, std::ios_base::app);

    QString stFileAddNamePic = QString("%1").arg(saveDirectory.c_str());
    //    //qDebug() <<  stFileAddNamePic ;
    QString stFileAddName22 = QString("%1.txt").arg(stFileAddNamePic);
    //   //qDebug() <<  stFileAddName22 ;
    QString date_name = Utilities::newdateName();
    QString time_name = Utilities::newtimeName();

    QString plateNumber1 = QString("%1").arg(_t_car.str_plat1.c_str());
    QStringList items = plateNumber1.split("\n");

    QString plateNumber2 = QString("%1").arg(_t_car.str_plat1.c_str());
    QStringList items2 = plateNumber2.split("\n\n");

    QString plateNumber3 = QString("%1").arg(_t_car.str_plat3.c_str());
    QStringList items3 = plateNumber3.split("\n");


    if(QRegExp(".+\\.(txt)").exactMatch(stFileAddName22))
    {
        QFile file(stFileAddName22);
        if(file.open(QIODevice::WriteOnly | QIODevice::Append)){
            QTextStream out(&file);
            out<< date_name << ","
               << time_name  << ","
               << _t_car.str_brand.c_str() << ","
               << _t_car.str_color.c_str() << ","
               << items.at(0).toStdString().c_str() << ","
               << items2.at(0).toStdString().c_str()  << "\n,";
            //out << editor->toPlainText() << "\n";
        }
    }

}

bool MainWindow::find_large_contour1(cv::Mat &src, std::vector<cv::Point> &output)
{

    //qDebug() << "start find_large_contour1";

    cv::Mat gray, output_canny;
    //double alpha = 2.5; /*< Simple contrast control */
    int beta = -1500;       /*< Simple brightness control */
    //src.convertTo(src, -1, alpha, beta);

    //qDebug()<<"1";
    cv::Mat imgBlur;
    cv::blur(src,imgBlur, cv::Size(3,3));
    cv::cvtColor(imgBlur, gray, cv::COLOR_RGB2GRAY);
    cv::threshold(gray, gray, myThroshold_value1, 255, cv::THRESH_BINARY);
	
 	cv::Mat im_floodfill = gray.clone();
	cv::floodFill(im_floodfill, cv::Point(0, 0), cv::Scalar(255));
	cv::floodFill(im_floodfill, cv::Point(gray.cols - 1, gray.rows - 1), cv::Scalar(255));
	//cv::imshow("im_floodfill", im_floodfill);
	// Invert floodfilled image
	cv::Mat im_floodfill_inv;
	cv::bitwise_not(im_floodfill, im_floodfill_inv);
	// Combine the two images to get the foreground.
	cv::Mat im_out =  (gray | im_floodfill_inv);
	
    //cv::imshow("im_out",im_out);
    cv::cvtColor(im_out, gray, cv::COLOR_GRAY2RGB);
	
    int Canny_Throshold = 100;
    cv::Canny(gray, output_canny, Canny_Throshold, Canny_Throshold *3, 3, false);
	
	
    // find contours
    std::vector<std::vector<cv::Point> > contours;
    int max_area = 0;
    std::vector<cv::Point> large_contour;
	
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(output_canny, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_TC89_L1);

    //cv::findContours(output_canny, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    if (contours.size() == 0) return false;

    // find max area contours
	int largest_index = 0;
    for (unsigned int i = 0; i < contours.size(); ++i) {
        int area = (int)cv::contourArea(contours[i]);
        if (area > max_area) {
            large_contour = contours[i];
            max_area = area;
		largest_index = i;
        }
    }
    if (max_area == 0) return false;

    // simplify large contours
    cv::approxPolyDP(cv::Mat(large_contour), large_contour, 5, true);
    
	cv::Mat tempContours(output_canny.size(),CV_8UC1,cv::Scalar(0) );
	cv::drawContours(tempContours, contours, (int)largest_index, cv::Scalar(255, 255, 0), 2, cv::LINE_8, hierarchy, 0);

	// Create a vector to store lines of the image
	std::vector<Vec4i> lines;
	// Apply Hough Transform
    HoughLinesP(tempContours, lines, 1, CV_PI / 180, 120, 100, 50);
	// Draw lines on the image
	cv::Mat tempLine(src.size(), CV_8UC1, cv::Scalar(0));
    //std::cout << "line found " << lines.size() << std::endl;

	if (lines.size() < 2) return false;

	std::vector<cv::Point> vP;
	for (size_t i = 0; i < lines.size(); i++) {
		Vec4i l = lines[i];
		//line(tempShow, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 0, 255), 1);
		vP.push_back(Point(lines[i][0], lines[i][1]));
		vP.push_back(Point(lines[i][2], lines[i][3]));
	}

	for (int i = 0; i < vP.size()-2; i++) {
		line(tempLine, vP[i], vP[i+1], Scalar(255, 0, 255), 1);
	}

	line(tempLine, vP.front(), vP.back(), Scalar(255, 0, 255), 1);
	
	contours.clear();
	hierarchy.clear();
    cv::findContours(tempLine, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
	
	if (contours.size() == 0) return false;

	largest_index = 0;
	max_area = 0;

	// find max area contours
	for (unsigned int i = 0; i < contours.size(); ++i) {
		int area = (int)cv::contourArea(contours[i]);
		//std::cout << area << std::endl;
		if (area > max_area) {
			large_contour = contours[i];
			max_area = area;
			largest_index = i;

		}
	}
	if (max_area == 0) return false;
	
	// simplify large contours
	cv::approxPolyDP(cv::Mat(large_contour), large_contour, 5, true);
    // convex hull
	std::vector<cv::Point> convex_hull;
    cv::convexHull(large_contour, convex_hull, false);
    if (convex_hull.size() != 4) return false;

    cv::Point avgPoint(0, 0);
    for (int i = 0; i < convex_hull.size(); i++) {
        avgPoint += convex_hull[i];
    }
    avgPoint = avgPoint / 4;

    int start;

    if (convex_hull[0].y < avgPoint.y) {
        if (convex_hull[0].x > avgPoint.x) start = 3; //q1
        else start = 0; //q2
    }
    else {
        if (convex_hull[0].x < avgPoint.x) start = 1; //q3
        else start = 2; //q4
    }

    for (int i = 0; i < 4; i++) {
        if (start >= convex_hull.size()) start = 0;
        output.push_back(convex_hull[start]);
        start++;
    }





    //-----------------------------------------------------
    /* cv::Mat testImg(600,600,CV_8UC3,cv::Scalar::all(0));
    src.copyTo(testImg(cv::Rect(cv::Point(src.cols, 0), src.size())));
    gray.copyTo(testImg(cv::Rect(cv::Point(0, src.rows), gray.size())));

    cv::cvtColor(output_canny, output_canny, cv::COLOR_GRAY2RGB);
    for (int i = 0; i < output.size(); i++) {
        circle(output_canny, output[i], 2, cv::Scalar(255, 0, 0), 2);
    }

    output_canny.copyTo(testImg(cv::Rect(cv::Point(src.cols, src.rows), output_canny.size())));//----------------------------------------------------
    cv::String myText;
    myText = cv::format("output.size %d, dx %d > 100, dy %d > 100",
                        output.size(),
                        abs(output[2].x - output[0].x),
            abs(output[2].y - output[0].y));
    cv::putText(testImg,myText,cv::Point(20,400),1,1,cv::Scalar::all(255),1);//-----------------------
    cv::imshow("cannyCheck",testImg);//----------------------------------------------------
    cv::waitKey(10);*/
    //----------------------------------------------------
    //----------------------------------------------------------------------------------


    if (abs(output[2].x - output[0].x) > 70 && abs(output[2].y - output[0].y) > 30) {
        //        cv::cvtColor(output_canny, output_canny, cv::COLOR_GRAY2RGB);
        //        for (int i = 0; i < output.size(); i++) {
        //            circle(output_canny, output[i], 2, cv::Scalar(255, 0, 0), 2);
        //        }
        //output_canny.copyTo(myFrame.imgCar(cv::Rect(cv::Point(src.cols, src.rows), output_canny.size())));
        //qDebug() << "find_large_contour1 is return true";
        return true;
    }
    else {
        //qDebug() << "find_large_contour1 is return false";
        return false;
    }


}

cv::Mat MainWindow::detectTextAreas(QImage &image, std::vector<cv::Rect> &areas)
{
    float confThreshold = 0.6;
    float nmsThreshold = 0.3;
    int inputWidth = 160;
    int inputHeight = 160;
    std::string model = "C:/Users/moudp/Documents/Literacy/frozen_east_text_detection.pb";
    //std::string model = "C:/Users/moudp/Desktop/output TensorFlow 2/saved_model.pb";
    // Load DNN network.
    if (net.empty()) {
        net = cv::dnn::readNet(model);
    }

    std::vector<cv::Mat> outs;
    std::vector<std::string> layerNames(2);
    layerNames[0] = "feature_fusion/Conv_7/Sigmoid";
    layerNames[1] = "feature_fusion/concat_3";

    cv::Mat frame = cv::Mat(
                image.height(),
                image.width(),
                CV_8UC3,
                image.bits(),
                image.bytesPerLine()).clone();
    cv::Mat blob;

    cv::dnn::blobFromImage(
                frame, blob,
                1.0, cv::Size(inputWidth, inputHeight),
                cv::Scalar(123.68, 116.78, 103.94), true, false
                );
    net.setInput(blob);
    net.forward(outs, layerNames);

    cv::Mat scores = outs[0];
    cv::Mat geometry = outs[1];

    std::vector<cv::RotatedRect> boxes;
    std::vector<float> confidences;
    decode(scores, geometry, confThreshold, boxes, confidences);

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

    // Render detections.
    cv::Point2f ratio((float)frame.cols / inputWidth, (float)frame.rows / inputHeight);
    cv::Scalar green = cv::Scalar(0, 255, 0);

    for (size_t i = 0; i < indices.size(); ++i) {
        cv::RotatedRect& box = boxes[indices[i]];
        cv::Rect area = box.boundingRect();
        area.x *= ratio.x;
        area.width *= ratio.x*1.1;
        area.y *= ratio.y;
        area.height *= ratio.y*1.1;
        areas.push_back(area);
        cv::rectangle(frame, area, green, 1);
        QString index = QString("%1").arg(i);
        //        cv::putText(
        //                    frame, index.toStdString(), cv::Point2f(area.x, area.y - 2),
        //                    cv::FONT_HERSHEY_SIMPLEX, 0.5, green, 1
        //                    );

    }

    return frame;

}





void MainWindow::decode(const cv::Mat& scores, const cv::Mat& geometry, float scoreThresh,
                        std::vector<cv::RotatedRect>& detections, std::vector<float>& confidences)
{
    CV_Assert(scores.dims == 4); CV_Assert(geometry.dims == 4);
    CV_Assert(scores.size[0] == 1); CV_Assert(scores.size[1] == 1);
    CV_Assert(geometry.size[0] == 1);  CV_Assert(geometry.size[1] == 5);
    CV_Assert(scores.size[2] == geometry.size[2]);
    CV_Assert(scores.size[3] == geometry.size[3]);

    detections.clear();
    const int height = scores.size[2];
    const int width = scores.size[3];
    for (int y = 0; y < height; ++y) {
        const float* scoresData = scores.ptr<float>(0, 0, y);
        const float* x0_data = geometry.ptr<float>(0, 0, y);
        const float* x1_data = geometry.ptr<float>(0, 1, y);
        const float* x2_data = geometry.ptr<float>(0, 2, y);
        const float* x3_data = geometry.ptr<float>(0, 3, y);
        const float* anglesData = geometry.ptr<float>(0, 4, y);
        for (int x = 0; x < width; ++x) {
            float score = scoresData[x];
            if (score < scoreThresh)
                continue;

            // Decode a prediction.
            // Multiple by 4 because feature maps are 4 time less than input image.
            float offsetX = x * 4.0f, offsetY = y * 4.0f;
            float angle = anglesData[x];
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            float h = x0_data[x] + x2_data[x];
            float w = x1_data[x] + x3_data[x];

            cv::Point2f offset(offsetX + cosA * x1_data[x] + sinA * x2_data[x],
                               offsetY - sinA * x1_data[x] + cosA * x2_data[x]);
            cv::Point2f p1 = cv::Point2f(-sinA * h, -cosA * h) + offset;
            cv::Point2f p3 = cv::Point2f(-cosA * w, sinA * w) + offset;
            cv::RotatedRect r(0.5f * (p1 + p3), cv::Size2f(w, h), -angle * 180.0f / (float)CV_PI);
            detections.push_back(r);
            confidences.push_back(score);
        }
    }
}

void MainWindow::appendSavedPhoto(QString name)
{
    QString photo_path = Utilities::getPhotoPath(name, "jpg");
    QString photo_name = Utilities::newPhotoName();
    QListWidgetItem *itm = new QListWidgetItem(tr(photo_name.toStdString().c_str()));
    itm->setIcon(QPixmap(photo_path).scaledToHeight(380));
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setResizeMode(QListView::Adjust);
    ui->listWidget->setSpacing(5);
    ui->listWidget->setWrapping(false);
    ui->listWidget->addItem(itm);
}

void MainWindow::takePhoto(cv::Mat &frame)
{
    QString photo_name = Utilities::newPhotoName();
    QString photo_path = Utilities::getPhotoPath(photo_name, "jpg");
    cv::imwrite(photo_path.toStdString(), frame);
    emit photoTaken(photo_name);
    taking_photo = false;
    Auto_taking_photo =false;
}

void MainWindow::data_config()
{
    qDebug()<<"hello";
    ConfigurationDialog dialog(this);
    if(dialog.exec() == QDialog::Rejected) {
     return; // do nothing if dialog rejected
    }
    plate_w = dialog.plateWidth().toInt();
    plate_h = dialog.plateHeight().toInt();
    myThroshold_value1 =dialog.plateThreshold().toInt();
    alpha = dialog.plateContrast().toDouble();

}

std::vector<MainWindow::ananColor> MainWindow::load_colorlist()
{
    std::vector<ananColor> _tempV_color;
    std::ifstream myfile(loadColorListDirectory);

    if (myfile.is_open())
    {
        int idx1 = 0;
        int idx2 = 0;

        ananColor _t_color;
        std::string line;

        while (std::getline(myfile, line))
        {
            idx1 = 0;

            idx2 = line.find(",", idx1);
            _t_color.name = line.substr(idx1, idx2 - idx1);
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.hue_min = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.hue_max = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.sat_min = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.sat_max = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.val_min = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_color.val_max = std::atoi(line.substr(idx1, idx2 - idx1).c_str());
            idx1 = idx2 + 1;

            _tempV_color.push_back(_t_color);
        }
        myfile.close();
    }
    return _tempV_color;
}

std::vector<MainWindow::Car> MainWindow::load_backlist()
{
    std::vector<Car> _tempV_car;
    std::ifstream myfile(loadCarBlackListDirectory);

    if (myfile.is_open())
    {
        int idx1 = 0;
        int idx2 = 0;

        Car _t_car;
        std::string line;

        while (std::getline(myfile, line))
        {
            idx1 = 0;

            idx2 = line.find(",", idx1);
            _t_car.str_brand = line.substr(idx1, idx2 - idx1);
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_car.str_color = line.substr(idx1, idx2 - idx1);
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_car.str_plat1 = line.substr(idx1, idx2 - idx1);
            idx1 = idx2 + 1;

            idx2 = line.find(",", idx1);
            _t_car.str_plat2 = line.substr(idx1, idx2 - idx1);

            _tempV_car.push_back(_t_car);
        }
        myfile.close();
    }
    return _tempV_car;
}


void MainWindow::RunGui()
{
    cv::Rect roi_color_detect(cv::Point(250,120),cv::Point(370,190));
    cv::Rect roi_plate_detect(cv::Point(100, 180),cv::Point(450, 450));

    //        cv::Rect roi_color_detect(cv::Point(200,190),cv::Point(300,220));
    //        cv::Rect roi_plate_detect(cv::Point(60, 230),cv::Point(350, 450));

    if(_server54000.myFrame.mbYolo)
    {
        //cap>>temp;
        temp = _server54000.getImage();
        std::vector<bbox_t> result_vec = detector->detect(temp);
        result_vec = detector->tracking_id(result_vec);	// comment it - if track_id is not required
        //draw_boxes(frame, result_vec, obj_names);
        int boxNumber = result_vec.size();
        myCar.isDetect = false;
        myFrame.imgCar = cv::Scalar::all(0);
        for (int k = 0; k < boxNumber; k++)
        {
            bbox_t _tembBox = result_vec[k];
            cv::Scalar color(0, 255, 255);

//                        cv::rectangle(temp, cv::Rect(_tembBox.x-5, _tembBox.y-10, _tembBox.w+20, _tembBox.h+10), color, 1);
//                        putText(temp,
//                                obj_names[_tembBox.obj_id],
//                                cv::Point2f(_tembBox.x + 5, _tembBox.y - 5),
//                                cv::FONT_HERSHEY_COMPLEX_SMALL,
//                                0.6,
//                                color);

            if (_tembBox.obj_id == 0) //0 plate number
            {
                //qDebug()<< _tembBox.obj_id;
                myCar.Px = _tembBox.x + _tembBox.w / 2;
                myCar.Py = _tembBox.y + _tembBox.h / 2;

                cv::Point _pp1 = cv::Point(myCar.Px, myCar.Py) - cv::Point(_tembBox.w, _tembBox.h) / 2 - cv::Point(5, 5);
                cv::Point _pp2 = cv::Point(myCar.Px, myCar.Py) + cv::Point(_tembBox.w, _tembBox.h) / 2 + cv::Point(10, 10);

                //qDebug()<<"pp1="<<_pp1.x<<","<<_pp1.y<<"pp2="<<_pp2.x<<","<<_pp2.y;

                if (_pp1.inside(roi_plate_detect) && _pp2.inside(roi_plate_detect) && _tembBox.w >=50)
                {
                    //cv::imshow("ccc",temp(roi_color_detect).clone());
                    CountTime++;
                    detectCarColor(temp(roi_color_detect).clone());

                    // Display Color
                    ui->Color->setText(myCar.str_color.c_str());

                    // Display Plate Number
                    cv::Mat may_be_Plate = temp(cv::Rect(_pp1, _pp2)).clone();
                    QImage framePlate(
                                may_be_Plate.data,
                                may_be_Plate.cols,
                                may_be_Plate.rows,
                                may_be_Plate.step,
                                QImage::Format_RGB888);
                    QPixmap imagePlate = QPixmap::fromImage(framePlate);
                    ui->ShowImage_Plate->setPixmap(imagePlate);

                    may_be_Plate.copyTo(myFrame.imgCar);

                    //qDebug()<< " detectCarPlateXX2";

                    if (detectCarPlate(may_be_Plate))
                    {
                        //qDebug()<< " detectCarPlateXX2";

                        char *old_ctype = strdup(setlocale(LC_ALL, NULL));
                        setlocale(LC_ALL, "C");
                        if (TBapi == nullptr) {
                            TBapi = new tesseract::TessBaseAPI();
                        }

                        myCar.isDetect = true;
                        taking_photo = true;
                        TBapi->SetImage(
                                    myFrame.imgPlate.data,
                                    myFrame.imgPlate.cols,
                                    myFrame.imgPlate.rows,
                                    3,
                                    myFrame.imgPlate.step);


                        QImage imgPlate2(
                                    myFrame.imgPlate.data,
                                    myFrame.imgPlate.cols,
                                    myFrame.imgPlate.rows,
                                    myFrame.imgPlate.step,
                                    QImage::Format_RGB888);

                        /*std::vector<cv::Rect> areas;
                        cv::Mat newImage = detectTextAreas(imgPlate2, areas);
                        //cv::imshow("crop",newImage);
                        QImage framePlate2(
                                    newImage.data,
                                    newImage.cols,
                                    newImage.rows,
                                    newImage.step,
                                    QImage::Format_RGB888);
                        QPixmap imagePlate = QPixmap::fromImage(imgPlate2);
                        ui->ShowImage_Plate_2->setPixmap(imagePlate)*/;

                        //qDebug()<< " detectCarPlateXXX";
                        if(CountTime > 10){

                            if(taking_photo)
                            {
                                takePhoto(temp);
                            }

                            std::string tText = std::string(TBapi->GetUTF8Text());
                            qDebug()<<tText.c_str();
                            myCar.str_plat1 = tText;

                            //ui->plainTextEdit->setPlainText("");
                            int min = 100;
                            int max = 0;
                            int i_province = 0;
                            int i_number = 0;
                            int i_char = 0;

//                            for(int i = 0; i<areas.size();i++){
//                                if(areas[i].y > max){
//                                    max = areas[i].y;
//                                    //province
//                                    i_province = i;
//                                }

//                                if(areas[i].x < min){
//                                    min = areas[i].x;
//                                    //plate charactor
//                                    i_char = i;
//                                }
//                            }

//                            i_number = 3 - i_province - i_char;

//                            TBapi->SetRectangle(areas[i_char].x, areas[i_char].y, areas[i_char].width, areas[i_char].height);
//                            char *outText1 = TBapi->GetUTF8Text();
//                            //myCar.str_plat1 = std::string(outText);
//                            if(sizeof(outText1)/sizeof(char) > 1) myCar.str_plat1 = std::string(outText1);
//                            //ui->plainTextEdit->setPlainText(ui->plainTextEdit->toPlainText()+myCar.str_plat1.c_str());
//                            qDebug()<<outText1;
//                            delete [] outText1;

//                            TBapi->SetRectangle(areas[i_number].x, areas[i_number].y, areas[i_number].width, areas[i_number].height);
//                            char *outText2 = TBapi->GetUTF8Text();
//                            if(sizeof(outText2)/sizeof(char) > 1) myCar.str_plat2 =  std::string(outText2);
//                            //myCar.str_plat1 = std::string(outText);
//                            //ui->plainTextEdit->setPlainText(ui->plainTextEdit->toPlainText()+myCar.str_plat2.c_str());
//                            delete [] outText2;

//                            TBapi->SetRectangle(areas[i_province].x, areas[i_province].y, areas[i_province].width, areas[i_province].height);
//                            char *outText3 = TBapi->GetUTF8Text();
//                            if(sizeof(outText3)/sizeof(char) > 1) myCar.str_plat3 = std::string(outText3);
//                            //myCar.str_plat1 = std::string(outText);
//                            //ui->plainTextEdit->setPlainText(ui->plainTextEdit->toPlainText()+myCar.str_plat3.c_str());
//                            delete [] outText3;

                            saveCarData(myCar);

                        }
                        CountTime = 0;
                        setlocale(LC_ALL, old_ctype);
                        free(old_ctype);
                        //qDebug()<<myCar.str_plat1.c_str();


                        //Black list
                        car_BlackList_index = -1;
                        for (int i = 0; i < myBlacklist.size(); i++) {
                            if (myCar.str_plat1 == myBlacklist[i].str_plat1)
                            {
                                cv::String _tempStr("Black List");
                                qDebug()<< _tempStr.c_str();
                                cv::putText(
                                            myFrame.imgCar,
                                            _tempStr,
                                            cv::Point(50, 200),
                                            1,
                                            3,
                                            cv::Scalar(0, 0, 255),
                                            2);
                                car_BlackList_index = i;
                            }
                        }

                    }

                }

            } else {
                //qDebug()<< _tembBox.obj_id;
                myCar.Px = _tembBox.x + _tembBox.w / 2;
                myCar.Py = _tembBox.y + _tembBox.h / 2;

                cv::Point _pp1 = cv::Point(myCar.Px, myCar.Py) - cv::Point(_tembBox.w, _tembBox.h) / 2 - cv::Point(10, 10);
                cv::Point _pp2 = cv::Point(myCar.Px, myCar.Py) + cv::Point(_tembBox.w, _tembBox.h) / 2 + cv::Point(20, 20);

                //qDebug()<<"pp1="<<_pp1.x<<","<<_pp1.y<<"pp2="<<_pp2.x<<","<<_pp2.y;

                ui->brand->setText("N/A");
                if (_pp1.inside(roi_plate_detect) && _pp2.inside(roi_plate_detect))
                {
                    // Display Plate Number
                    cv::Mat may_be_Bland = temp(cv::Rect(_pp1, _pp2)).clone();
                    cvtColor(may_be_Bland, may_be_Bland, cv::COLOR_BGR2RGB);
                    QImage frameBland(
                                may_be_Bland.data,
                                may_be_Bland.cols,
                                may_be_Bland.rows,
                                may_be_Bland.step,
                                QImage::Format_RGB888);
                    QPixmap imageBland = QPixmap::fromImage(frameBland);
                    ui->ShowImage_Bland->setPixmap(imageBland);
                    //qDebug()<< _tembBox.obj_id;

                    // Car Brand
                    switch (_tembBox.obj_id) {
                    case 1:
                        ui->brand->setText("HONDA");
                        myCar.str_brand = "HONDA";
                        break;
                    case 2:
                        ui->brand->setText("TOYOTA");
                        myCar.str_brand = "TOYOTA";
                        break;
                    default:
                        ui->brand->setText("N/A");
                        myCar.str_brand = "N/A";
                    }

                }

            }

        }

        // Display Plate Number
        //QString plateNumber = QString("%1").arg(myCar.str_plat1.c_str());
        QString plateNumber = QString("%1").arg(myCar.str_plat1.c_str());
        QStringList items = plateNumber.split("\n,");
        //qDebug()<<items;
        ui->PlateNumber->setText(items.at(0));

//        //QString plateNumber = QString("%1").arg(myCar.str_plat1.c_str());
//        QString plateNumber2 = QString("%1").arg(myCar.str_plat1.c_str());
//        QStringList items3 = plateNumber2.split("\n,");
//        //qDebug()<<items;
//        ui->PlateNumber2->setText(items3.at(0));

        // Display Province
        QString province = QString("%1").arg(myCar.str_plat1.c_str());
        QStringList items2 = plateNumber.split("\n\n");
        //ui->PlateNumber_Province->setText(items2.at(0));
               for(const QString& item: items2) {
                    ui->PlateNumber_Province->setText(item);
               }

        cvtColor(temp, temp, cv::COLOR_BGR2RGB);
        QImage frame(
                    temp.data,
                    temp.cols,
                    temp.rows,
                    temp.step,
                    QImage::Format_RGB888);

        QPixmap image = QPixmap::fromImage(frame);
        ui->ShowImage->setPixmap(image);

    }
    //qDebug()<< " stop";
    myCar.isDetect = false;
    _server54000.myFrame.mbYolo = false;
    capturePlate = false;

}

