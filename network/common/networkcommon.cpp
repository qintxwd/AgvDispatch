#include "networkcommon.h"

using namespace qyhnetwork;

#ifdef WIN32
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "psapi")
#pragma comment(lib, "Mswsock")
#endif

namespace qyhnetwork
{

NetworkEnvironment::NetworkEnvironment()
{
#ifdef WIN32
    WORD version = MAKEWORD(2,2);
    WSADATA d;
    if (WSAStartup(version, &d) != 0)
    {
        assert(0);
    }
#endif
}
NetworkEnvironment::~NetworkEnvironment()
{
#ifdef WIN32
    WSACleanup();
#endif
}

NetworkEnvironment g_networkEnvironment;

std::string getHostByName(const std::string & name)
{
    if (std::find_if(name.begin(), name.end(), [](char ch) {return !isdigit(ch) && ch != '.'; }) == name.end())
    {
        return name; //ipv4
    }
    if (std::find_if(name.begin(), name.end(), [](char ch) {return !isxdigit(ch) && ch != ':'; }) == name.end())
    {
        return name; //ipv6
    }
    struct addrinfo *res = nullptr;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(name.c_str(), "3306", &hints, &res) == 0)
    {
        char buf[100] = { 0 };
        if (res->ai_family == AF_INET)
        {
            inet_ntop(res->ai_family, &(((sockaddr_in*)res->ai_addr)->sin_addr), buf, 100);
        }
        else if (res->ai_family == AF_INET6)
        {
            inet_ntop(res->ai_family, &(((sockaddr_in6*)res->ai_addr)->sin6_addr), buf, 100);
        }
        return buf;
    }

    return "";
}

std::string getPureHostName(const std::string & host)
{
    static std::string ipv6pre = "::ffff:";
    if (host.length() > ipv6pre.length() && host.compare(0, ipv6pre.length(), ipv6pre) == 0)
    {
        return host.substr(ipv6pre.length());
    }
    return host;
}

}
