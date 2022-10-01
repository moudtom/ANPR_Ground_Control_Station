#ifndef TCPSERVERCLASS_H
#define TCPSERVERCLASS_H

#include <QObject>
#include <QThread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <fstream>
#include <vector>
#include <mutex>
#include <QTimer>
// Need to link with Ws2_32.lib


#pragma comment (lib, "Ws2_32.lib")
class tcpServerClass:public QThread
{
    Q_OBJECT
public:
    tcpServerClass();
    void WriteData(const char *input);
    cv::Mat getImage();
    void setPort(int DEFAULT_PORT,int DEFAULT_BUFLEN);

    QString _str_server_status;
    QString commandDataFromServer;
    QByteArray datasFromClient;

    int clientCount;
    bool bServerStart;
    bool bConnected;
    bool bShowImage;
    unsigned int countTimeRead;
    unsigned int countTimeWrite;
    std::string ip_address;
    bool bConnect = false;

    struct moudtomframe
    {
        cv::Mat imgALL_M, imgCar_M, imgPlate_M, imgColor_M;
        bool mbYolo, mbTCP;

        moudtomframe() {
            mbYolo = false;
        }

    }myFrame;
private:
    void run();
    SOCKET client;
    int DEFAULT_PORT;
    int DEFAULT_BUFLEN;
    std::mutex myMtx;
    std::string serverStatus = "";
    cv::Mat outputImage;
};

#endif // TCPSERVERCLASS_H
