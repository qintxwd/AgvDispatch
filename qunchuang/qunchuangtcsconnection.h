#ifndef QUNCHUANGTCSCONNECTION_H
#define QUNCHUANGTCSCONNECTION_H

#include <string>
class QyhTcpClient;

class QunChuangTcsConnection
{
public:
    QunChuangTcsConnection(std::string _ip, int _port);
    ~QunChuangTcsConnection();
    void init();

    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();
    void send(char *data,int len);
private:
    QyhTcpClient *tcpClient;
    std::string ip;
    int port;
};

#endif // QUNCHUANGTCSCONNECTION_H
