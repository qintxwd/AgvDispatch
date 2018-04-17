#ifndef COMMON_IMPL_H
#define COMMON_IMPL_H

#include "../common/networkcommon.h"

#ifndef WIN32
#include <sys/epoll.h>
namespace qyhnetwork
{
const int MAX_EPOLL_WAIT = 5000;
class TcpSocket;
class TcpAccept;
const int InvalidFD = -1;
struct EventData
{
    enum REG_TYPE
    {
        REG_INVALID,
        REG_ZSUMMER,
        REG_TCP_SOCKET,
        REG_TCP_ACCEPT,
    };

    epoll_event   _event; //event, auto set
    unsigned char _type = REG_INVALID; //register type
    unsigned char _linkstat = LS_UNINITIALIZE;
    int              _fd = InvalidFD;   //file descriptor
    std::shared_ptr<TcpSocket> _tcpSocketPtr;
    std::shared_ptr<TcpAccept> _tcpacceptPtr;
};

template <class T>
T& operator <<(T &t, const EventData & reg)
{
    t << "registerEvent Info: epoll_event.events[" << reg._event.events
      << "] _type[" << (int)reg._type << "] _linkstat[" << (int)reg._linkstat
      << "] _fd[" << reg._fd << "]";
    return t;
}
}

#endif

#endif // COMMON_IMPL_H
