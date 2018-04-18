#ifndef EPOLLACCEPT_H
#define EPOLLACCEPT_H


#include "epoll_common.h"
#include "epoll.h"
#include "epollsocket.h"

#ifndef WIN32

namespace qyhnetwork
{
class TcpAccept : public std::enable_shared_from_this<TcpAccept>
{
public:
    TcpAccept();
    ~TcpAccept();
    bool initialize(const EventLoopPtr &summer);
    bool openAccept(const std::string & listenIP, unsigned short listenPort, bool reuse = true);
    bool doAccept(const TcpSocketPtr &s, _OnAcceptHandler &&handle);
    void onEPOLLMessage(uint32_t event);
    bool close();

private:
    std::string logSection();
private:
    EventLoopPtr         _summer;
    std::string        _listenIP;
    short            _listenPort = 0;
    bool _isIPV6 = false;


    EventData _eventData;
    _OnAcceptHandler _onAcceptHandler;
    TcpSocketPtr  _client;

};
using TcpAcceptPtr = std::shared_ptr<TcpAccept>;

}

#endif


#endif // EPOLLACCEPT_H
