#ifndef IOCPSOCKET_H
#define IOCPSOCKET_H

#include "iocp_common.h"
#include "iocp.h"

#ifdef WIN32
namespace qyhnetwork
{
using _OnRecvHandler = std::function<unsigned int(NetErrorCode, int)>;
using _OnConnectHandler = std::function<void(NetErrorCode)>;

class TcpSocket : public std::enable_shared_from_this<TcpSocket>
{
public:

    TcpSocket();
    ~TcpSocket();

    bool initialize(const EventLoopPtr& summer);
    bool setNoDelay();
    inline bool getPeerInfo(std::string& remoteIP, unsigned short &remotePort)
    {
        remoteIP = _remoteIP;
        remotePort = _remotePort;
        return true;
    }

    bool doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler);
    bool doSend(char * buf, unsigned int len);
    bool doRecv(char * buf, unsigned int len, _OnRecvHandler && handler);
    bool doClose();
public:
    bool attachSocket(SOCKET s, std::string remoteIP, unsigned short remotePort, bool isIPV6);
    void onIOCPMessage(BOOL bSuccess, DWORD dwTranceBytes, unsigned char cType);
    std::string logSection();
public:
    //private
    EventLoopPtr  _summer;
    SOCKET        _socket = INVALID_SOCKET;
    std::string _remoteIP;
    unsigned short _remotePort = 0;
    bool _isIPV6 = false;
    //recv
    ExtendHandle _recvHandle;
    WSABUF         _recvWSABuf;
    _OnRecvHandler _onRecvHandler;

    //send
    ExtendHandle _sendHandle;
    WSABUF         _sendWsaBuf;

    //connect
    ExtendHandle _connectHandle;
    _OnConnectHandler _onConnectHandler;
    //status
    int _linkStatus = LS_UNINITIALIZE;
};
using TcpSocketPtr = std::shared_ptr<TcpSocket>;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;
}

#endif


#endif // IOCPSOCKET_H
