#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QLabel>
#include <fstream>
#include <vector>
#include<iostream>
#include <tcpserverclass.h>
#include <allheaders.h> // leptonica main header for image io
#include <tesseract/baseapi.h> // tesseract main header

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui_c.h>

#define OPENCV

#include <yolo_v2_class.hpp>
Detector  detector();
using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    tcpServerClass _server54000;
    std::string names_file = "C:/darknet-master/data/obj.names";
    std::string cfg_file = "C:/darknet-master/cfg/yolo-obj.cfg";
    std::string weights_file = "C:/darknet-master/yolo-obj_plate.weights";
    std::vector<bbox_t> result_vect;
    vector<string> objects_names_from_file(string const filename);
    vector<string> obj_names ;
    Detector* detector;
    struct ananframe
    {
        cv::Mat imgALL, imgCar, imgPlate, imgColor;
        bool bYolo, bTCP;
        ananframe() {
            bYolo = false;
        }

    }myFrame;

public slots:
    void RunGui();
    void openCamera();

private:
    Ui::MainWindow *ui;
    QTimer         *tmrTimer; // timer
    QStatusBar     *mainStatusBar;
    QLabel         *mainStatusLabel;

    //OpenCV
    //VideoCapture    cap;
     cv::Mat        temp;
};
#endif // MAINWINDOW_H
