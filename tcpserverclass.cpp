#include "tcpserverclass.h"
#include "QtDebug"


tcpServerClass::tcpServerClass()
{
    outputImage = cv::Mat::zeros(cv::Size(640,480), CV_8UC3);
    DEFAULT_PORT =   50000;
    DEFAULT_BUFLEN = 2000000;
    countTimeRead =  0;
    countTimeWrite = 0;
    bConnected =     false;
    bServerStart =   true;
    _str_server_status = "";
    commandDataFromServer = "";
    datasFromClient = "";
    clientCount = 0;
    bShowImage = false;
}

void tcpServerClass::setPort(int _port,int _buflen)
{
    DEFAULT_PORT = _port;
    DEFAULT_BUFLEN = _buflen;
}

void tcpServerClass::WriteData(const char *input){
    send(client,input,strlen(input),0);
    commandDataFromServer = input;
}

cv::Mat tcpServerClass::getImage(){
     cv::Mat _tempMat;
     myMtx.lock();
     outputImage.copyTo(_tempMat);
     myMtx.unlock();
    return _tempMat;
}

void tcpServerClass::run(){

//    cv::Rect roi_color_detectx(cv::Point(200,190),cv::Point(300,220));
//    cv::Rect roi_plate_detectx(cv::Point(60, 230),cv::Point(350, 450));
        cv::Rect roi_color_detectx(cv::Point(250,100),cv::Point(370,170));
        cv::Rect roi_plate_detectx(cv::Point(100, 180),cv::Point(450, 450));
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(2, 2);
    WSAStartup(version, (LPWSADATA)&wsaData);
    qDebug()<< "create socket";
    //create the socket
    SOCKET theSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (theSocket == SOCKET_ERROR)
    {
        qDebug()<< "Failed socket()";
        serverStatus = "Failed socket()";
        bConnect = false;
        return ;
    }

    //Fill in the sockaddr_in struct
    SOCKADDR_IN serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(54000);
    inet_pton(AF_INET, ip_address.c_str(), &serverInfo.sin_addr);


    serverStatus = "waitting...";
    qDebug()<< "waitting connect to server";
    if (::connect(theSocket, (LPSOCKADDR)&serverInfo, sizeof(serverInfo)) == SOCKET_ERROR)
    {
        serverStatus = "Failed connect()";
        qDebug()<< "Failed connect()";
        bConnect = false;
        return ;
    }

    qDebug()<< "connect()";

    cv::Mat _img = cv::Mat::zeros(cv::Size(640, 480), CV_8UC3); //CV_8UC1);
    const int imgSize = _img.total() * _img.elemSize();
    uchar* imgBuff = _img.data;


    //send(theSocket, "a", 1, 0);
    int myCount = 0;
    qDebug()<< bConnect;

    while (bConnect)
    {
        //qDebug()<<"loop";
        _img = cv::Scalar(0, 0, 0);
        send(theSocket, "a", 1, 0);
        Sleep(20);
        int bytes = recv(theSocket, (char*)imgBuff, imgSize, MSG_WAITALL); //flag MSG_WAITALL  - do not complete until packet is completely filled */

        if (bytes != SOCKET_ERROR)
        {
            send(theSocket, "a", 1, 0);


            if(!myFrame.mbYolo){
                myMtx.lock();
//                cv::rectangle(_img, roi_color_detectx, cv::Scalar(255, 0, 0), 1);
//                cv::rectangle(_img, roi_plate_detectx, cv::Scalar(0, 255, 0), 1);
                //cv::rectangle(_img, cv::Rect(0,30,150,50),cv::Scalar::all(0),-1);
                //cv::putText(_img, std::to_string(bytes) + "/" + std::to_string((imgSize)), cv::Point(10,50),1,1,cv::Scalar(255,255,255),1);
                _img.copyTo(outputImage);
                myFrame.mbYolo = true;
                myMtx.unlock();
            }

        }
        else {
            bConnect = false;
            WSACleanup();
            break;
        }
        Sleep(5);
    }

    serverStatus = "disconnect";
    closesocket(theSocket);
    //cout << "closing client" << endl;
    WSACleanup();
    return ;
}
