#include "realagv.h"
#include "qyhtcpclient.h"

RealAgv::RealAgv(int id,std::string name,std::string ip,int port):
    Agv(id,name,ip,port),
    tcpClient(nullptr)
{

}

RealAgv::~RealAgv(){
    if (tcpClient)delete tcpClient;
}

void RealAgv::init()
{
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&RealAgv::onRead, this, std::placeholders::_1, std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&RealAgv::onConnect, this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&RealAgv::onDisconnect, this);
    tcpClient = new QyhTcpClient(ip, port, onread, onconnect, ondisconnect);
}

void RealAgv::onRead(const char *data, int len)
{

}

void RealAgv::onConnect()
{
    //TODO
}

void RealAgv::onDisconnect()
{
    //TODO
}


void RealAgv::reconnect()
{
    tcpClient->resetConnect(ip, port);
}


bool RealAgv::send(const char *data, int len)
{
    return tcpClient->sendToServer(data, len);
}
