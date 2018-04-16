
#ifndef UDPSOCKET_IMPL_H_
#define UDPSOCKET_IMPL_H_

#include "common_impl.h"
#include "iocp_impl.h"

#ifdef WIN32

namespace qyhnetwork
{
class UdpSocket : public std::enable_shared_from_this<UdpSocket>
{
public:
    UdpSocket();
    ~UdpSocket();
    bool initialize(const EventLoopPtr &summer, const char *localIP, unsigned short localPort);
    bool doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport);
    bool onIOCPMessage(BOOL bSuccess, DWORD dwTranceBytes, unsigned char cType);
    bool doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler &&handler);
public:
    //private
    EventLoopPtr _summer;

    SOCKET        _socket;
    SOCKADDR_IN    _addr;

    //recv
    ExtendHandle _recvHandle;
    WSABUF         _recvWSABuf;
    sockaddr_in  _recvFrom;
    int             _recvFromLen;
    _OnRecvFromHandler _onRecvHander;
    LINK_STATUS _linkStatus;
};
using UdpSocketPtr = std::shared_ptr<UdpSocket>;
}

#endif
#endif











