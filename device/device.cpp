#include "device.h"

Device::Device(int _id, std::string _name, std::string _ip, int _port) :
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    tcpClient(nullptr)
{
    connected = false;
}

Device::~Device()
{
    if (tcpClient)
        delete tcpClient;
}

bool Device::init()
{
    TcpClient::QyhClientReadCallback onread = std::bind(&Device::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::QyhClientConnectCallback onconnect = std::bind(&Device::onConnect, this);
    TcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&Device::onDisconnect, this);
    tcpClient = new TcpClient(ip, port, onread, onconnect, ondisconnect);
    return tcpClient != nullptr ? true : false;
}

void Device::unInit()
{
    if (tcpClient)
    {
        delete tcpClient;
    }
}

bool Device::send(const char *data, int len)
{
  return tcpClient->sendToServer(data, len);
}

void Device::onRead(const char *data,int len)
{

}

void Device::onConnect()
{
    connected = true;
}

void Device::onDisconnect()
{
    connected = false;
}

void Device::reconnect()
{
    tcpClient->resetConnect(ip, port);
}

void Device::bytesToHexstring(const char* bytes,int bytelength,char *hexstring,int hexstrlength)
{
  char str2[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
  for (int i=0,j=0;i<bytelength,j<hexstrlength;i++,j++)
  {
           int b;
           b = 0x0f&(bytes[i]>>4);
           char s1 = str2[b];
           hexstring[j] = s1;
           b = 0x0f & bytes[i];
           char s2 = str2[b];
           j++;
           hexstring[j] = s2;
  }
}



