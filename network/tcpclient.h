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

    typedef std::function<void (const char *data,int len)> ClientReadCallback;
    typedef std::function<void ()> ClientConnectCallback;
    typedef std::function<void ()> ClientDisconnectCallback;

    TcpClient(std::string _ip,int _port, ClientReadCallback _readcallback, ClientConnectCallback _connectcallback, ClientDisconnectCallback _disconnectcallback);

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

    ClientReadCallback readcallback;

    ClientConnectCallback connectcallback;

    ClientDisconnectCallback disconnectcallback;

    bool quit;

    boost::asio::io_service io_context;

//    tcp::socket s;
    boost::shared_ptr<tcp::socket> s;

    std::thread t;
};

#endif // TCPCLIENT_H
