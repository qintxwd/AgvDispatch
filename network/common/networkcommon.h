#ifndef NETWORKCOMMON_H
#define NETWORKCOMMON_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32")
#else
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#endif

#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <utility>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>

namespace qyhnetwork
{
    //! NetErrorCode
    enum NetErrorCode
    {
        NEC_SUCCESS = 0,
        NEC_ERROR,
        NEC_REMOTE_CLOSED,
    };

    enum LINK_STATUS
    {
        LS_UNINITIALIZE, //socket default status
        LS_WAITLINK, // socket status after init and will to connect.
        LS_ATTACHED, // socket attached will to initialize.
        LS_ESTABLISHED, //socket status is established
        LS_CLOSED, // socket is closed. don't use it again.
    };

    class NetworkEnvironment
    {
    public:
        NetworkEnvironment();
        ~NetworkEnvironment();
        inline void addCreatedSocketCount(){ _totalCreatedCTcpSocketObjs++; }
        inline void addClosedSocketCount(){ _totalClosedCTcpSocketObjs++; }
        inline unsigned int getCreatedSocketCount(){ return _totalCreatedCTcpSocketObjs; }
        inline unsigned int getClosedSocketCount(){ return _totalClosedCTcpSocketObjs; }
    private:
        std::atomic_uint _totalCreatedCTcpSocketObjs;
        std::atomic_uint _totalClosedCTcpSocketObjs;
    };

    extern NetworkEnvironment g_networkEnvironment;

    std::string getHostByName(const std::string & name);
    std::string getPureHostName(const std::string & host);
#ifndef WIN32
    inline bool setNonBlock(int fd) {return fcntl((fd), F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK) == 0;}
    inline bool setNoDelay(int fd){int bTrue = true?1:0; return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0;}
    inline bool setReuse(int fd){int bReuse = 1;return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &bReuse, sizeof(bReuse)) == 0;}
    inline bool setIPV6Only(int fd, bool only){int ipv6only = only ? 1 : 0;return setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only, sizeof(ipv6only)) == 0;}
#else
    inline bool setNonBlock(SOCKET s) {unsigned long val = 1;return ioctlsocket(s, FIONBIO, &val) == NO_ERROR;}
    inline bool setNoDelay(SOCKET s){BOOL bTrue = TRUE;return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0;}
    inline bool setReuse(SOCKET s){BOOL bReUseAddr = TRUE;return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&bReUseAddr, sizeof(BOOL)) == 0;}
    inline bool setIPV6Only(SOCKET s, bool only){DWORD ipv6only = only ? 1: 0;return setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only)) == 0;}
#endif
}

#endif // NETWORKCOMMON_H
