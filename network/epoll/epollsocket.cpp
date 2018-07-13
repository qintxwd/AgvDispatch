#include "epollsocket.h"

#include "../../utils/Log/spdlog/spdlog.h"
#include "../../common.h"
using namespace qyhnetwork;

#ifndef WIN32

TcpSocket::TcpSocket()
{
    _eventData._event.data.ptr = &_eventData;
    _eventData._event.events = 0;
    _eventData._type = EventData::REG_TCP_SOCKET;
    g_networkEnvironment.addCreatedSocketCount();
}


TcpSocket::~TcpSocket()
{
    g_networkEnvironment.addClosedSocketCount();
    if (_onRecvHandler || _onConnectHandler)
    {
        combined_logger->warn("TcpSocket::~TcpSocket Handler status error. " );
    }
    if (_eventData._fd != InvalidFD)
    {
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
    }
}

std::string TcpSocket::logSection()
{
    std::stringstream os;
    os << ";; Status: summer.user_count()=" << _summer.use_count() << ", remoteIP=" << _remoteIP << ", remotePort=" << _remotePort
        << ", _onConnectHandler = " << (bool)_onConnectHandler
        << ", _onRecvHandler = " << (bool)_onRecvHandler << ", _recvBuf=" << (void*)_recvBuf << ", _recvLen=" << _recvLen
        << ", _sendBuf=" << (void*)_sendBuf << ", _sendLen=" << _sendLen
        << "; _eventData=" << _eventData;
    return os.str();
}

bool TcpSocket::setNoDelay()
{
    return qyhnetwork::setNoDelay(_eventData._fd);
}

bool TcpSocket::initialize(const EventLoopPtr& summer)
{

    if (_eventData._linkstat == LS_ATTACHED)
    {
        _summer = summer;
        _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
        _eventData._tcpSocketPtr = shared_from_this();
        _eventData._linkstat = LS_ESTABLISHED;
        setNonBlock(_eventData._fd);
        return true;
    }
    else if (_eventData._linkstat == LS_UNINITIALIZE)
    {
        _summer = summer;
        _eventData._linkstat = LS_WAITLINK;
        return true;
    }

    combined_logger->error("TcpSocket::initialize fd aready used!");
    return true;
}
bool TcpSocket::attachSocket(int fd, const std::string & remoteIP, unsigned short remotePort, bool isIPV6)
{
    _eventData._fd = fd;
    _remoteIP = remoteIP;
    _remotePort = remotePort;
    _isIPV6 = isIPV6;
    _eventData._linkstat = LS_ATTACHED;
    return true;
}




bool TcpSocket::doConnect(const std::string& remoteIP, unsigned short remotePort, _OnConnectHandler && handler)
{
    if (!_summer || _eventData._linkstat != LS_WAITLINK || _onConnectHandler)
    {
        combined_logger->error("TcpSocket::doConnect status error!");
        return false;
    }
    _remoteIP = remoteIP;
    _remotePort = remotePort;

    _isIPV6 = remoteIP.find(':') != std::string::npos;
    int ret = -1;
    if (_isIPV6)
    {
        _eventData._fd = socket(AF_INET6, SOCK_STREAM, 0);
        if (_eventData._fd == -1)
        {
            combined_logger->error("TcpSocket::doConnect fd create failed!");
            return false;
        }
        setNonBlock(_eventData._fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, remoteIP.c_str(), &addr.sin6_addr);
        addr.sin6_port = htons(_remotePort);
        ret = connect(_eventData._fd, (sockaddr *)&addr, sizeof(addr));
    }
    else
    {

        _eventData._fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_eventData._fd == -1)
        {
            combined_logger->error("TcpSocket::doConnect fd create failed!");
            return false;
        }
        setNonBlock(_eventData._fd);
        _remoteIP = remoteIP;
        _remotePort = remotePort;
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(_remoteIP.c_str());
        addr.sin_port = htons(_remotePort);
        ret = connect(_eventData._fd, (sockaddr *)&addr, sizeof(addr));
    }

    if (ret!=0 && errno != EINPROGRESS)
    {
        combined_logger->warn("TcpSocket::doConnect  error. errno={0}" , strerror(errno));
        ::close(_eventData._fd);
        _eventData._fd = InvalidFD;
        return false;
    }

    _eventData._event.events = EPOLLOUT;
    _summer->registerEvent(EPOLL_CTL_ADD, _eventData);
    _eventData._tcpSocketPtr = shared_from_this();
    _onConnectHandler = std::move(handler);
    return true;
}


bool TcpSocket::doSend(char * buf, unsigned int len)
{
//    combined_logger->debug("TcpSocket::doSend len={0}" , len);
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        combined_logger->warn("TcpSocket::doSend _linkstat error!");
        return false;
    }

    if (!_summer)
    {
        combined_logger->error("TcpSocket::doSend _summer not bind!");
        return false;
    }

    if (buf == NULL || len == 0)
    {
        combined_logger->error("TcpSocket::doSend (_sendBuf == NULL || _sendLen == 0) == TRUE");
        return false;
    }

    int ret = 0;

    ret = send(_eventData._fd, buf, len, 0);

    if (ret <= 0)
    {
        _sendBuf = buf;
        _sendLen = len;

        _eventData._event.events = _eventData._event.events|EPOLLOUT;
        _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
    }
    else
    {
//        combined_logger->debug("TcpSocket::doSend direct sent={0}" , ret);
    }
    return true;
}


bool TcpSocket::doRecv(char * buf, unsigned int len, _OnRecvHandler && handler, bool daemonRecv)
{
    if (_eventData._linkstat != LS_ESTABLISHED)
    {
        combined_logger->warn("TcpSocket::doRecv status error !");
        return false;
    }

    if (!_summer)
    {
        combined_logger->error("TcpSocket::doRecv  _summer not bind!" );        return false;
    }

    if (len == 0 )
    {
        combined_logger->error("TcpSocket::doRecv argument err !!!  len==0");
        return false;
    }
    if (_recvBuf != NULL || _recvLen != 0)
    {
        combined_logger->error("TcpSocket::doRecv  (_recvBuf != NULL || _recvLen != 0) == TRUE" );
        return false;
    }
    if (_onRecvHandler)
    {
        combined_logger->error("TcpSocket::doRecv (_onRecvHandler) == TRUE");
        return false;
    }
    if (_daemonRecv)
    {
        combined_logger->error("TcpSocket::doRecv already open daemon recv");
        return false;
    }
    if (daemonRecv)
    {
        _daemonRecv = daemonRecv;
    }

    _recvBuf = buf;
    _recvLen = len;
    _recvOffset = 0;

    _eventData._event.events = _eventData._event.events|EPOLLIN;
    _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
    _onRecvHandler = std::move(handler);
    return true;
}


void TcpSocket::onEPOLLMessage(uint32_t event)
{
    NetErrorCode ec = NEC_ERROR;
    if (!_onRecvHandler && !_onConnectHandler)
    {
        combined_logger->error("TcpSocket::onEPOLLMessage unknown error. errno={0}",strerror(errno));
        return ;
    }
    if (_eventData._linkstat != LS_WAITLINK && _eventData._linkstat != LS_ESTABLISHED)
    {
        combined_logger->error("TcpSocket::onEPOLLMessage _linkstat error. _linkstat={0}" ,_eventData._linkstat);
        return;
    }

    if (_eventData._linkstat == LS_WAITLINK)
    {
        auto guard = shared_from_this();
        _OnConnectHandler onConnect(std::move(_onConnectHandler));
        if (event & EPOLLERR || event & EPOLLHUP)
        {
            doClose();
            onConnect(NEC_ERROR);
            return;
        }
        else if(event & EPOLLOUT)
        {
            _eventData._event.events = 0;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            _eventData._linkstat = LS_ESTABLISHED;
            onConnect(NEC_SUCCESS);
            return;
        }
        return ;
    }

    if (_onRecvHandler)
    {
        int ret = -1;
        errno = EAGAIN;
        if (event & EPOLLIN)
        {
            ret = recv(_eventData._fd, _recvBuf + _recvOffset, _recvLen - _recvOffset, 0);
        }
        if (event & EPOLLHUP || event & EPOLLERR || ret == 0 || (ret == -1 && (errno != EAGAIN && errno != EWOULDBLOCK) ) )
        {
            _eventData._event.events = _eventData._event.events &~EPOLLIN;
            _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
            if (ret == 0 || event & EPOLLHUP)
            {
                ec = NEC_REMOTE_CLOSED;
            }
            else
            {
                ec = NEC_ERROR;
            }
            _OnRecvHandler onRecv(std::move(_onRecvHandler));
            doClose();
            onRecv(ec, 0);
            return ;
        }
        else if (ret != -1)
        {
            if (_daemonRecv)
            {
                auto guard = shared_from_this();
                _recvOffset = _onRecvHandler(NEC_SUCCESS,ret);
            }
            else
            {
                _eventData._event.events = _eventData._event.events &~EPOLLIN;
                _summer->registerEvent(EPOLL_CTL_MOD, _eventData);
                auto guard = shared_from_this();
                _OnRecvHandler onRecv(std::move(_onRecvHandler));
                _recvBuf = NULL;
                _recvLen = 0;
                _recvOffset = 0;
                onRecv(NEC_SUCCESS,ret);
            }

            if (_eventData._linkstat == LS_CLOSED)
            {
                return;
            }
        }
    }
    return ;
}


bool TcpSocket::doClose()
{
    if (_eventData._linkstat != LS_CLOSED)
    {
        _eventData._linkstat = LS_CLOSED;
        _summer->registerEvent(EPOLL_CTL_DEL, _eventData);
        if (_eventData._fd != InvalidFD)
        {
            shutdown(_eventData._fd, SHUT_RDWR);
            close(_eventData._fd);
            _eventData._fd = InvalidFD;
        }
        _onConnectHandler = nullptr;
        _onRecvHandler = nullptr;
        _eventData._tcpSocketPtr.reset();
    }
    return true;
}

#endif
