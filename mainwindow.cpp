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

    // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel(mainStatusBar);
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("ANPR Air Task Force 9 Program is Ready");
}

MainWindow::~MainWindow()
{
    //_server54000.terminate();
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

void MainWindow::RunGui()
{

    if(_server54000.myFrame.mbYolo){
        //cap>>temp;

        temp = _server54000.getImage();
        std::vector<bbox_t> result_vec = detector->detect(temp,0.7);
        result_vec = detector->tracking_id(result_vec);	// comment it - if track_id is not required
//        //draw_boxes(frame, result_vec, obj_names);
        int boxNumber = result_vec.size();
        for (int k = 0; k < boxNumber; k++)
        {
             bbox_t _tembBox = result_vec[k];
              cv::Scalar color(60, 160, 260);
              cv::rectangle(temp, cv::Rect(_tembBox.x, _tembBox.y, _tembBox.w, _tembBox.h), color, 3);
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

    _server54000.myFrame.mbYolo = false;
}
