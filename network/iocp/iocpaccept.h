#ifndef IOCPACCEPT_H
#define IOCPACCEPT_H

#include "iocp_common.h"
#include "iocp.h"

#ifdef WIN32
namespace qyhnetwork
{

class TcpSocket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;
//accept callback
using _OnAcceptHandler = std::function<void(NetErrorCode, TcpSocketPtr)>;

class TcpAccept : public std::enable_shared_from_this<TcpAccept>
{
public:
    TcpAccept();
    ~TcpAccept();
    bool initialize(EventLoopPtr& summer);
    bool openAccept(const std::string ip, unsigned short port, bool reuse = true);
    bool doAccept(const TcpSocketPtr& s, _OnAcceptHandler &&handler);
    bool onIOCPMessage(BOOL bSuccess);
    bool close();
private:
    std::string logSection();
private:
    //config
    EventLoopPtr _summer;


    std::string        _ip;
    unsigned short    _port = 0;

    //listen
    SOCKET            _server = INVALID_SOCKET;
    bool              _isIPV6 = false;

    //client
    SOCKET _socket = INVALID_SOCKET;
    char _recvBuf[200];
    DWORD _recvLen = 0;
    ExtendHandle _handle;
    _OnAcceptHandler _onAcceptHandler;
    TcpSocketPtr _client;

};
using TcpAcceptPtr = std::shared_ptr<TcpAccept>;

}

#endif

#endif // IOCPACCEPT_H
