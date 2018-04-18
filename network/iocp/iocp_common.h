#ifndef IOCP_COMMON_H
#define IOCP_COMMON_H

#include "../common/networkcommon.h"

#ifdef WIN32
namespace qyhnetwork
{

class TcpSocket;
class TcpAccept;

const int InvalidFD = -1;
struct ExtendHandle
{
    OVERLAPPED     _overlapped;
    unsigned char _type;
    std::shared_ptr<TcpSocket> _tcpSocket;
    std::shared_ptr<TcpAccept> _tcpAccept;
    enum HANDLE_TYPE
    {
        HANDLE_ACCEPT,
        HANDLE_RECV,
        HANDLE_SEND,
        HANDLE_CONNECT,
    };
};
template <class T>
T & operator <<(T &t, const ExtendHandle & h)
{
    t << (unsigned int)h._type;
    return t;
}

#define HandlerFromOverlaped(ptr)  ((ExtendHandle*)((char*)ptr - (char*)&((ExtendHandle*)NULL)->_overlapped))
}

#endif


#endif // IOCP_COMMON_H
