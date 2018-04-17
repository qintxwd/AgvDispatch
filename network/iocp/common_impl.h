#ifndef COMMON_IMPL_H
#define COMMON_IMPL_H

#include "../common/networkcommon.h"
#ifdef WIN32
namespace qyhnetwork
{
class TcpSocket;
class TcpAccept;
class UdpSocket;
const int InvalidFD = -1;
struct ExtendHandle
{
    OVERLAPPED     _overlapped;
    unsigned char _type;
    std::shared_ptr<TcpSocket> _tcpSocket;
    std::shared_ptr<TcpAccept> _tcpAccept;
    std::shared_ptr<UdpSocket> _udpSocket;
    enum HANDLE_TYPE
    {
        HANDLE_ACCEPT,
        HANDLE_RECV,
        HANDLE_SEND,
        HANDLE_CONNECT,
        HANDLE_RECVFROM,
        HANDLE_SENDTO,
    };
};
template <class T>
T & operator <<(T &t, const ExtendHandle & h)
{
    t << (unsigned int)h._type;
    return t;
}
enum POST_COM_KEY
{
    PCK_USER_DATA,
};
#define HandlerFromOverlaped(ptr)  ((ExtendHandle*)((char*)ptr - (char*)&((ExtendHandle*)NULL)->_overlapped))
}

#endif
#endif // COMMON_IMPL_H
