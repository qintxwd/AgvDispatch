#include "qunchuangtcsconnection.h"
#include "../qyhtcpclient.h"
#include "../taskmaker.h"
#include "../common.h"
#include <string.h>

QunChuangTcsConnection::QunChuangTcsConnection(std::string _ip, int _port):
    ip(_ip),
    port(_port),
    tcpClient(nullptr)
{

}
QunChuangTcsConnection::~QunChuangTcsConnection(){
    if (tcpClient)delete tcpClient;
}

void QunChuangTcsConnection::init() {
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&QunChuangTcsConnection::onRead, this, std::placeholders::_1, std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&QunChuangTcsConnection::onConnect, this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&QunChuangTcsConnection::onDisconnect, this);
    tcpClient = new QyhTcpClient(ip, port, onread, onconnect, ondisconnect);
}

void QunChuangTcsConnection::send(char *data,int len)
{
    if(tcpClient!=nullptr)
        tcpClient->sendToServer(data,len);
}

void QunChuangTcsConnection::onRead(const char *data, int len)
{
#define MAX_MSG_LEN     (1024*10)
    static std::string recv_buffer = "";
    recv_buffer += std::string(data,len);

    while(true){
        std::size_t startPos = recv_buffer.find_first_of("*");
        std::size_t endPos = recv_buffer.find_first_of("#");

        if(startPos!=std::string::npos && endPos!=std::string::npos){
            if(endPos>startPos+1){
                std::string msg = recv_buffer.substr(startPos+1,endPos-startPos-1);
                //TODO:解析字符串
                combined_logger->info("TCS RECV:"+msg);


                {
                    std::string from;
                    std::string to;
                    int ceid;
                    int dispatch_id;
                    //TODO
                    //TaskMaker::getInstance()->makeTask(from,to,dispatch_id,ceid);
                }

            }
            //清理已经处理的数据
            recv_buffer = recv_buffer.substr(endPos+1);
        }else{
            break;
        }
    }
}

void QunChuangTcsConnection::onConnect()
{
    //TODO
}

void QunChuangTcsConnection::onDisconnect()
{
    //TODO
}
