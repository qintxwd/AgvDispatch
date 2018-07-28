#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <memory>
#include <functional>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class TcpClient;
using TcpClientPtr = std::shared_ptr<TcpClient>;

class TcpClient
{
public:
    enum{
        QYH_TCP_CLIENT_RECV_BUF_LEN = 1500,
    };

    typedef std::function<void (const char *data,int len)> QyhClientReadCallback;
    typedef std::function<void ()> QyhClientConnectCallback;
    typedef std::function<void ()> QyhClientDisconnectCallback;

    TcpClient(std::string _ip,int _port, QyhClientReadCallback _readcallback, QyhClientConnectCallback _connectcallback, QyhClientDisconnectCallback _disconnectcallback);

    ~TcpClient();

    void start();

    void resetConnect(std::string _ip, int _port);

    bool sendToServer(const char *data,int len);

    void disconnect();

    void threadProcess();

    void doClose();
private:
    bool need_reconnect;//重连

    std::string ip;

    int port;

    QyhClientReadCallback readcallback;

    QyhClientConnectCallback connectcallback;

    QyhClientDisconnectCallback disconnectcallback;

    bool quit;

    boost::asio::io_context io_context;

    tcp::socket s;

    std::thread t;
};

#endif // TCPCLIENT_H
