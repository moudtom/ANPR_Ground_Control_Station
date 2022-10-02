#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <opencv2/opencv.hpp>
#include <QTimer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tmrTimer = new QTimer(this);
    tmrTimer->start(30);

    // connect the signals and slots
    connect(ui->actionExit, SIGNAL(triggered(bool)), QApplication::instance(), SLOT(quit()));
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(openCamera()));

    //tcp ip
    _server54000.myFrame.mbYolo = false;
    //_yolothread.myFrameYolo.bYolo=false;

    // Initial Yolo
    obj_names = objects_names_from_file(names_file);
    detector = new Detector(cfg_file ,weights_file);
    //Initial Tessaract
    TBapi = new tesseract::TessBaseAPI();
    TBapi->Init("C:/Program Files/Tesseract-OCR/tessdata", "tha");
    TBapi->SetPageSegMode(tesseract::PSM_AUTO);

    // load color list
    myColorList = load_colorlist();

    // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel(mainStatusBar);
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("ANPR Air Task Force 9 Program is Ready");
}

MainWindow::~MainWindow()
{
    _server54000.terminate();
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
        _server54000.bConnect = true;
        _server54000.start();
        mainStatusLabel->setText("ANPR Ground Control Station is Connected");
    }
    connect(tmrTimer,SIGNAL(timeout()),this,SLOT(RunGui()));
    //cap = VideoCapture(0,CAP_DSHOW);

}

void MainWindow::detectCarColor(cv::Mat inputImg)
{
    //if (inputImg.empty()) return "";

    cv::GaussianBlur(inputImg, inputImg, cv::Size(5, 5), 0, 0);
    myDetectColor.findHistrogram(inputImg);
    myFrame.imgColor = myDetectColor.color_rgb;
    myCar.colorHSV = myDetectColor.color_HSV;

    QImage frameColor(
                myFrame.imgColor.data,
                myFrame.imgColor.cols,
                myFrame.imgColor.rows,
                myFrame.imgColor.step,
                QImage::Format_RGB888);
    QPixmap imageColor = QPixmap::fromImage(frameColor);
    ui->ShowImage_Color->setPixmap(imageColor);

    int _vh = (int)myCar.colorHSV.val[0];
    int _vs = (int)myCar.colorHSV.val[1];
    int _vv = (int)myCar.colorHSV.val[2];
    //qDebug()<<myColorList.size();

    for (int i = 1; i < myColorList.size(); i++) {

        if (_vh > myColorList[i].hue_min && _vh < myColorList[i].hue_max &&
                _vs > myColorList[i].sat_min && _vs < myColorList[i].sat_max &&
                _vv > myColorList[i].val_min && _vv < myColorList[i].val_max)
        {
            myCar.str_color = myColorList[i].name;
            qDebug()<<"myCar.str_color.c_str()";
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
        /*for (int i = 0; i < outPoint.size(); i++) {
                circle(inputImg, outPoint[i], 2, cv::Scalar(255, 0, 0), 2);
            }*/
        //qDebug()<<"find contour";
        cv::Point2f srcPoints[] = {
            outPoint[0],
            outPoint[1],
            outPoint[2],
            outPoint[3],
        };

        cv::Point2f dstPoints[] = {
            cv::Point(0,0),
            cv::Point(180, 0),
            cv::Point(180, 85),
            cv::Point(0, 85),
        };

        cv::Mat tempImg, tempImg_TS, tempImgGray;
        cv::Mat myMatrix = cv::getPerspectiveTransform(srcPoints, dstPoints);
        cv::warpPerspective(inputImg, tempImg, myMatrix, myFrame.imgPlate.size());

        QImage framePlate2(
                    tempImg.data,
                    tempImg.cols,
                    tempImg.rows,
                    tempImg.step,
                    QImage::Format_RGB888);
        QPixmap imagePlate2 = QPixmap::fromImage(framePlate2);
        ui->ShowImage_Plate_2->setPixmap(imagePlate2);


        //if(tempImg.rows > 90 && tempImg.cols > 180)
        tempImg.copyTo(myFrame.imgPlate);


        //if(tempImg.rows > 100 && tempImg.cols > 226)
        //tempImg (cv::Rect(0, 0, 226, 100)).copyTo(myFrame.imgPlate);

        return true;
    }
    else {
        myFrame.imgPlate = cv::Scalar(0, 0, 0);
        return false;
    }

}

bool MainWindow::find_large_contour1(cv::Mat &src, std::vector<cv::Point> &output)
{
    cv::Mat gray, output_canny;

    double alpha = 2.5; /*< Simple contrast control */
    int beta = -200;       /*< Simple brightness control */
    src.convertTo(src, -1, alpha, beta);
    //src.copyTo(myFrame.imgCar(cv::Rect(cv::Point(src.cols, 0), src.size())));
    //qDebug()<<"1";
    cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
    cv::threshold(gray, gray, myThroshold_value1, 255, cv::THRESH_BINARY);

    cv::cvtColor(gray, gray, cv::COLOR_GRAY2RGB);
    //gray.copyTo(myFrame.imgCar(cv::Rect(cv::Point(0, src.rows), gray.size())));

    int Canny_Throshold = 180;
    cv::Canny(gray, output_canny, Canny_Throshold, Canny_Throshold *3, 3, false);

    // find contours
    std::vector<std::vector<cv::Point> > contours;
    int max_area = 0;
    std::vector<cv::Point> large_contour;
    std::vector<cv::Point> convex_hull;
    cv::findContours(output_canny, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    if (contours.size() == 0) return false;

    // find max area contours
    for (unsigned int i = 0; i < contours.size(); ++i) {
        int area = (int)cv::contourArea(contours[i]);
        if (area > max_area) {
            large_contour = contours[i];
            max_area = area;
        }
    }
    if (max_area == 0) return false;

    // simplify large contours
    cv::approxPolyDP(cv::Mat(large_contour), large_contour, 5, true);

    cv::Mat tempShow = src.clone();

    // convex hull
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

    if (abs(output[2].x - output[0].x) > 100 && abs(output[2].y - output[0].y) > 50) {
        cv::cvtColor(output_canny, output_canny, cv::COLOR_GRAY2RGB);
        for (int i = 0; i < output.size(); i++) {
            circle(output_canny, output[i], 2, cv::Scalar(255, 0, 0), 2);
        }
        //output_canny.copyTo(myFrame.imgCar(cv::Rect(cv::Point(src.cols, src.rows), output_canny.size())));

        return true;
    }
    else {
        return false;
    }
}

std::vector<MainWindow::ananColor> MainWindow::load_colorlist()
{
    std::vector<ananColor> _tempV_color;
        std::ifstream myfile("C:/Users/moudp/Documents/ANPR_Ground_Control_Station_V2_1/color_data.csv");

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

void MainWindow::RunGui()
{
    cv::Rect roi_color_detect(cv::Point(250,100),cv::Point(370,170));
    cv::Rect roi_plate_detect(cv::Point(100, 250),cv::Point(450, 450));

    if(_server54000.myFrame.mbYolo)
    {
        //cap>>temp;
        temp = _server54000.getImage();
        std::vector<bbox_t> result_vec = detector->detect(temp);
        result_vec = detector->tracking_id(result_vec);	// comment it - if track_id is not required
        //        //draw_boxes(frame, result_vec, obj_names);
        int boxNumber = result_vec.size();
        myCar.isDetect = false;
        myFrame.imgCar = cv::Scalar::all(0);
        for (int k = 0; k < boxNumber; k++)
        {
            bbox_t _tembBox = result_vec[k];
            cv::Scalar color(60, 160, 260);
            //cv::rectangle(temp, cv::Rect(_tembBox.x, _tembBox.y, _tembBox.w, _tembBox.h), color, 3);

            if (_tembBox.obj_id == 0) //0 person, 1 bicycle, 2 car, 3 motorbike
            {

                //qDebug()<< _tembBox.obj_id;
                myCar.Px = _tembBox.x + _tembBox.w / 2;
                myCar.Py = _tembBox.y + _tembBox.h / 2;

                cv::Point _pp1 = cv::Point(myCar.Px, myCar.Py) - cv::Point(_tembBox.w, _tembBox.h) / 2 - cv::Point(10, 10);
                cv::Point _pp2 = cv::Point(myCar.Px, myCar.Py) + cv::Point(_tembBox.w, _tembBox.h) / 2 + cv::Point(20, 20);

                //qDebug()<<"pp1="<<_pp1.x<<","<<_pp1.y<<"pp2="<<_pp2.x<<","<<_pp2.y;

                if (_pp1.inside(roi_plate_detect) && _pp2.inside(roi_plate_detect))
                {
                    //cv::imshow("ccc",temp(roi_color_detect).clone());
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
                        myCar.isDetect = true;
                        //qDebug()<< " detectCarPlateXXX";
                        TBapi->SetImage(
                                    myFrame.imgPlate.data,
                                    myFrame.imgPlate.cols,
                                    myFrame.imgPlate.rows,
                                    3,
                                    myFrame.imgPlate.step);
                        std::string tText = std::string(TBapi->GetUTF8Text());
                        std::string tText2[2] = { "","" };
                        myCar.str_plat1 = tText;
                        //qDebug()<<myCar.str_plat1.c_str();

                        // Display Plate Number
                        QString plateNumber = QString("%1").arg(myCar.str_plat1.c_str());
                        //qDebug()<<record;
                        QStringList items = plateNumber.split("\n");
                        ui->PlateNumber->setText(items.at(0));
                        //ui->PlateNumber->setText(myCar.str_plat1.c_str());

                        // Display Province
                        QString province = QString("%1").arg(myCar.str_plat1.c_str());
                        QStringList items2 = plateNumber.split("\n\n");
                        for(const QString& item: items2) {
                            ui->PlateNumber_Province->setText(item);
                        }

                    }
                }

            }
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
    _server54000.myFrame.mbYolo = false;

}
