#ifndef QYHTCPCLIENT_H
#define QYHTCPCLIENT_H

#ifdef  WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32")
#else
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#endif

#include <functional>
#include <string>
#include <thread>

class QyhTcpClient
{
public:

    enum{
        QYH_TCP_CLIENT_RECV_BUF_LEN = 1500,
    };

    typedef std::function<void (const char *data,int len)> QyhClientReadCallback;
    typedef std::function<void ()> QyhClientConnectCallback;
    typedef std::function<void ()> QyhClientDisconnectCallback;

    QyhTcpClient(std::string ip, int _port, QyhClientReadCallback _readcallback, QyhClientConnectCallback _connectcallback, QyhClientDisconnectCallback _disconnectcallback);
    ~QyhTcpClient();
    void resetConnect(std::string _ip, int _port);

    bool sendToServer(const char *data,int len);

    void disconnect();
private:
    bool need_reconnect;//重连

    bool initConnect();

    volatile bool quit;

    int socketFd;

    std::string serverip;

    int port;

    QyhClientReadCallback readcallback;

    QyhClientConnectCallback connectcallback;

    QyhClientDisconnectCallback disconnectcallback;

    std::thread t;
};

#endif // QYHTCPCLIENT_H
