#include "tcpserverclass.h"
#include "QtDebug"
cv::Rect roi_color_detectx(500, 100, 100, 100);
cv::Rect roi_plate_detectx(240, 180, 200, 100);
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

            if (recv(theSocket, (char*)imgBuff, imgSize, 0) != SOCKET_ERROR)
            {
                send(theSocket, "a", 1, 0);


                  if(!myFrame.mbYolo){
                    myMtx.lock();
                    _img.copyTo(outputImage);
                    myFrame.mbYolo = true;
                    myMtx.unlock();
                  }

//                _img.copyTo(myShowImg);

                cv::rectangle(_img, roi_color_detectx, cv::Scalar(255, 0, 0), 1);
                cv::rectangle(_img, roi_plate_detectx, cv::Scalar(0, 255, 0), 1);

                //cv::imshow(MY_WINDOW_RECIVE, _img);
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
