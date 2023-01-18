#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QLabel>
#include <QTime>
#include <QTextEdit>
#include <fstream>
#include <vector>
#include<iostream>
#include <tcpserverclass.h>
#include <allheaders.h> // leptonica main header for image io
#include <tesseract/baseapi.h> // tesseract main header
#include "opencv2/dnn.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui_c.h>
#include "configurationdialog.h"

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
    void takePhoto() {taking_photo = true; }

    tcpServerClass _server54000;
    std::string names_file = "C:/darknet-master/data/obj.names";
    std::string cfg_file = "C:/darknet-master/cfg/yolo-obj.cfg";
    std::string weights_file = "C:/darknet-master/yolo-obj-plate_final.weights";
    std::string saveDirectory = "C:/Users/moudp/Documents/ANPR_Ground_Control_Station_V2_1/save history";
    std::string loadCarBlackListDirectory = "C:/Users/moudp/Documents/ANPR_Ground_Control_Station_V2_1/blacklist_data.csv";
    std::string loadColorListDirectory = "C:/Users/moudp/Documents/ANPR_Ground_Control_Station_V2_1/color_data.csv";

    std::vector<bbox_t> result_vect;
    vector<string> objects_names_from_file(string const filename);
    vector<string> obj_names ;
    Detector* detector;
    tesseract::TessBaseAPI* TBapi;

    struct ananframe
    {
        cv::Mat imgALL, imgCar, imgPlate, imgColor;
        bool bYolo, bTCP;
        ananframe() {
            bYolo = false;
        }

    }myFrame;

    struct Car
    {
        cv::Mat img, colorMask;
        std::string str_brand;
        std::string str_color;
        std::string str_plat1;
        std::string str_plat2;
        std::string str_plat3;
        std::string str_date;
        std::string str_time;
        cv::Scalar colorRGB;
        cv::Scalar colorHSV;
        //CTime date_time;
        double Px, Py;
        bool isDetect;

        Car() {
            str_brand = "";
            str_color = "";
            str_plat1 = "";
            str_plat2 = "";
            str_plat3 = "";
            str_date = "";
            str_time = "";
            Px = 0;
            Py = 0;
            isDetect = false;
        }
    };Car myCar;

    struct ananDetectColor {
        std::vector<cv::Mat> hsv_hist;
        double minVal[3], maxVal[3];
        cv::Point minLoc[3], maxLoc[3];
        cv::Scalar color_rgb;
        cv::Scalar color_HSV;

        cv::Scalar hsv2bgrCvt(cv::Scalar hsv_color) {
            cv::Mat hsv(1, 1, CV_8UC3, hsv_color);
            cv::Mat bgr;
            cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
            return cv::Scalar((int)bgr.at<cv::Vec3b>(0, 0)[0], (int)bgr.at<cv::Vec3b>(0, 0)[1], (int)bgr.at<cv::Vec3b>(0, 0)[2]);
        }

        void findHistrogram(cv::Mat& input)
        {
            cv::Mat input_hsv;
            std::vector<cv::Mat> hsv_plate, _temp_hist;
            cv::cvtColor(input, input_hsv, cv::COLOR_BGR2HSV);
            cv::split(input_hsv, hsv_plate);

            int histSize = 256;
            float range[] = { 0, 256 }; //the upper boundary is exclusive
            const float* histRange[] = { range };
            bool uniform = true, accumulate = false;


            for (int i = 0; i < hsv_plate.size(); i++) {
                cv::Mat _tempImg;
                cv::calcHist(&hsv_plate[i], 1, 0, cv::Mat(), _tempImg, 1, &histSize, histRange, uniform, accumulate);
                cv::minMaxLoc(_tempImg, &minVal[i], &maxVal[i], &minLoc[i], &maxLoc[i]);
                _temp_hist.push_back(_tempImg);
            }

            color_HSV = cv::Scalar(maxLoc[0].y, maxLoc[1].y, maxLoc[2].y);
            color_rgb = hsv2bgrCvt(cv::Scalar(maxLoc[0].y, maxLoc[1].y, maxLoc[2].y));
            //qDebug()<<color_rgb.val;
            hsv_hist = _temp_hist;

        }


        cv::Mat DrawGraph() {
            int hist_w = 256, hist_h = 250, histSize = 256;;
            int bin_w = cvRound((double)hist_w / histSize);

            cv::Mat myHist[3];
            cv::Scalar _color(0, 0, 0);//
            cv::Mat histImage(hist_h * 3, hist_w, CV_8UC3, _color);

            for (int i = 0; i < hsv_hist.size(); i++) {
                cv::normalize(hsv_hist[i], myHist[i], 0, hist_h - 50, cv::NORM_MINMAX, -1, cv::Mat());
            }

            for (int i = 1; i < histSize; i++)
            {
                //_color = hsv2bgrCvt(cv::Scalar(i, maxLoc[1].y, maxLoc[2].y));
                _color = hsv2bgrCvt(cv::Scalar(i, 200, 200));

                rectangle(histImage,
                          cv::Point(i * bin_w, hist_h),
                          cv::Point((i + 1) * bin_w, hist_h - cvRound(myHist[0].at<float>(i))),
                        _color,
                        cv::FILLED);

                _color = hsv2bgrCvt(cv::Scalar(maxLoc[0].y, i, maxLoc[2].y));
                //_color = hsv2bgrCvt(cv::Scalar(maxLoc[0].y, i, 200));

                line(histImage,
                     cv::Point(bin_w * (i - 1), hist_h * 2 - cvRound(myHist[1].at<float>(i - 1))),
                        cv::Point(bin_w * (i), hist_h * 2 - cvRound(myHist[1].at<float>(i))),
                        _color, 2, 8, 0);

                line(histImage, cv::Point(bin_w * (i - 1), hist_h * 3 - cvRound(myHist[2].at<float>(i - 1))),
                        cv::Point(bin_w * (i), hist_h * 3 - cvRound(myHist[2].at<float>(i))),
                        cv::Scalar(i, i, i), 2, 8, 0);
            }

            _color = cv::Scalar::all(225);
            int tt = 0;
            double ttt = 0.6;
            cv::putText(histImage, "Hue " + std::to_string(maxLoc[0].y), cv::Point(0, 30), tt, ttt, _color);
            cv::putText(histImage, "Saturation " + std::to_string(maxLoc[1].y), cv::Point(0, hist_h + 30), tt, ttt, _color);
            cv::putText(histImage, "Value " + std::to_string(maxLoc[2].y), cv::Point(0, hist_h * 2 + 30), tt, ttt, _color);

            return histImage;
        }
    }myDetectColor;

    struct ananColor
    {
        std::string name;
        int hue_min;
        int hue_max;
        int sat_min;
        int sat_max;
        int val_min;
        int val_max;

        ananColor() {
            name = "-";
            hue_min = 0;
            hue_max = 0;
            sat_min = 0;
            sat_max = 0;
            val_min = 0;
            val_max = 0;
        }
    };

    bool bYoloThread = false;
    bool capturePlate = false;
    cv::Mat  frameSend;
    int myThroshold_value1 = 200;
    int myThroshold_value2 = 180;
    double alpha = 10;
    std::vector<ananColor> myColorList;
    std::vector<Car> myBlacklist;
    int car_BlackList_index = -1;
    int iSelectCar = 0;
    int iSelectColor = 0;


signals:
    void frameCaptured(cv::Mat *data);
    void photoTaken(QString name);
public slots:
    void RunGui();
    void openCamera();
    void detectCarColor(cv::Mat inputImg);
    bool detectCarPlate(cv::Mat &inputImg);
    void saveCarData(Car _t_car);
    bool find_large_contour1(cv::Mat &src, std::vector<cv::Point> &output);
    cv::Mat detectTextAreas(QImage &image, std::vector<cv::Rect>&);

    void decode(const cv::Mat& scores, const cv::Mat& geometry, float scoreThresh,
        std::vector<cv::RotatedRect>& detections, std::vector<float>& confidences);
private slots:
    void appendSavedPhoto(QString name);
    void takePhoto(cv::Mat &frame);
    void data_config();
private:
    Ui::MainWindow *ui;
    QTimer         *tmrTimer; // timer
    QStatusBar     *mainStatusBar;
    QLabel         *mainStatusLabel;

    //OpenCV
    //VideoCapture    cap;
    cv::Mat        temp;
    std::vector<ananColor> load_colorlist();
    std::vector<Car> load_backlist();
    cv::dnn::Net net;

    vector<string> classes;
        float confThreshold = 0.6;
        float nmsThreshold = 0.5;
          int inputWidth = 160;
          int inputHeight = 160;
    // take photos
    bool taking_photo;
    bool Auto_taking_photo;
    int CountTime=0;
    int CountNum=0;

    // text editor
    QTextEdit *editor;
    QString outputText;
    // palte size
    int plate_w  = 226 ;
    int plate_h  = 100 ;


};
#endif // MAINWINDOW_H
