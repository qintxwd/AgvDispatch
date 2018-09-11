#ifndef DEVICE_H
#define DEVICE_H
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include "../network/tcpclient.h"
#include "../userlogmanager.h"


using namespace std;

//DEVICE
class Device
{
public:
    Device(int id,std::string name,std::string ip,int port);
    Device();
    virtual ~Device();
    bool init();
    void unInit();
    void start();
    int getId(){return id;}
    string getIp(){return ip;}
    int getType(){return deviceType;}
    int setType(int _type){deviceType = _type;}

    bool isConnected()
    {
        return connected;
    }


protected:
    bool send(const char *data, int len);
    //回调
    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();
    virtual void reconnect();
    void bytesToHexstring(const char* bytes,int bytelength,char *hexstring,int hexstrlength);


protected:
    int id;
    std::string name;
    std::string ip;
    int port;
    bool connected;
    int deviceType;
    bool runFlag;
    TcpClient *tcpClient;
};

#endif // DEVICE_H
