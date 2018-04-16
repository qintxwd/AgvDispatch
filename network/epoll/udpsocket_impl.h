#ifndef UDPSOCKET_IMPL_H
#define UDPSOCKET_IMPL_H

#include "common_impl.h"
#include "epoll_impl.h"
#ifndef WIN32
namespace qyhnetwork{


class UdpSocket : public std::enable_shared_from_this<UdpSocket>
{
public:
    // const char * remoteIP, unsigned short remotePort, nTranslate
    UdpSocket();
    ~UdpSocket();
    bool initialize(const EventLoopPtr &summer, const char *localIP, unsigned short localPort);
    bool doRecvFrom(char * buf, unsigned int len, _OnRecvFromHandler&& handler);
    bool doSendTo(char * buf, unsigned int len, const char *dstip, unsigned short dstport);
    bool onEPOLLMessage(uint32_t event);
public:
    EventLoopPtr _summer;
    EventData _eventData;

    _OnRecvFromHandler _onRecvFromHandler;
    unsigned int _recvLen;
    char    *     _recvBuf;
};
using UdpSocketPtr = std::shared_ptr<UdpSocket>;


}
#endif
#endif // UDPSOCKET_IMPL_H
