#ifndef EPOLLSOCKET_H
#define EPOLLSOCKET_H

#include "epoll_common.h"
#include "epoll.h"

#ifndef WIN32
namespace qyhnetwork
{
using _OnRecvHandler = std::function<unsigned int(NetErrorCode, int)>;
using _OnConnectHandler = std::function<void(NetErrorCode)>;

class TcpSocket: public std::enable_shared_from_this<TcpSocket>
{
public:
    TcpSocket();
    ~TcpSocket();
    bool setNoDelay();
    inline void setFloodSendOptimize(bool enable){_floodSendOptimize = enable;}
    bool initialize(const EventLoopPtr& summer);
    inline bool getPeerInfo(std::string& remoteIP, unsigned short &remotePort)
    {
        remoteIP = _remoteIP;
        remotePort = _remotePort;
        return true;
    }
    bool doConnect(const std::string &remoteIP, unsigned short remotePort, _OnConnectHandler && handler);
    bool doSend(char * buf, unsigned int len);
    bool doRecv(char * buf, unsigned int len, _OnRecvHandler && handler, bool daemonRecv = false);
    bool doClose();


public:
	void onEPOLLMessage(uint32_t event);
    bool attachSocket(int fd, const std::string& remoteIP, unsigned short remotePort, bool isIPV6);
private:
    std::string logSection();
private:
    EventLoopPtr _summer;
    std::string _remoteIP;
    unsigned short _remotePort = 0;
    bool _isIPV6 = false;
    bool _daemonRecv = false;
    bool _floodSendOptimize = true;
    EventData _eventData;

    _OnConnectHandler _onConnectHandler;

    _OnRecvHandler _onRecvHandler;
    unsigned int _recvLen = 0;
    char    *     _recvBuf = NULL;
    unsigned int _recvOffset = 0;

    _OnSendHandler _onSendHandler;
    unsigned int _sendLen = 0;
    char *         _sendBuf = NULL;
};
using TcpSocketPtr = std::shared_ptr<TcpSocket>;

}
#endif

#endif // EPOLLSOCKET_H
