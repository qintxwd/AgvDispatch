#include "realagv.h"
#include "network/tcpclient.h"

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
    TcpClient::QyhClientReadCallback onread = std::bind(&RealAgv::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::QyhClientConnectCallback onconnect = std::bind(&RealAgv::onConnect, this);
    TcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&RealAgv::onDisconnect, this);
    tcpClient = new TcpClient(ip, port, onread, onconnect, ondisconnect);
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
