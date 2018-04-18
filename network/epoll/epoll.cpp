#include "epoll.h"
#include "epollsocket.h"
#include "epollaccept.h"

using namespace qyhnetwork;

#ifndef WIN32

bool EventLoop::initialize()
{
    if (_epoll != InvalidFD)
    {
        LOG(FATAL)<<"EventLoop::initialize[this0x"<<this <<"] epoll is created ! " );
        return false;
    }
    const int IGNORE_ENVENTS = 100;
    _epoll = epoll_create(IGNORE_ENVENTS);
    if (_epoll == InvalidFD)
    {
        LOG(FATAL)<<"EventLoop::initialize[this0x" << this << "] create epoll err errno=" << strerror(errno));
        return false;
    }

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, _sockpair) != 0)
    {
        LOG(FATAL)<<"EventLoop::initialize[this0x" << this << "] create socketpair.  errno=" << strerror(errno));
        return false;
    }
    setNonBlock(_sockpair[0]);
    setNonBlock(_sockpair[1]);
    setNoDelay(_sockpair[0]);
    setNoDelay(_sockpair[1]);

    _eventData._event.data.ptr = &_eventData;
    _eventData._event.events = EPOLLIN;
    _eventData._fd = _sockpair[1];
    _eventData._linkstat = LS_ESTABLISHED;
    _eventData._type = EventData::REG_ZSUMMER;
    registerEvent(EPOLL_CTL_ADD, _eventData);
    return true;
}

bool EventLoop::registerEvent(int op, EventData & ed)
{
    if (epoll_ctl(_epoll, op, ed._fd, &ed._event) != 0)
    {
        LOG(WARNING)<<"EventLoop::registerEvent error. op=" << op << ", event=" << ed._event.events);
        return false;
    }
    return true;
}

void EventLoop::PostMessage(_OnPostHandler &&handle)
{
    _OnPostHandler * pHandler = new _OnPostHandler(std::move(handle));
    bool needNotice = false;
    _postQueueLock.lock();
    if (_postQueue.empty()){needNotice = true;}
    _postQueue.push_back(pHandler);
    _postQueueLock.unlock();
    if (needNotice)
    {
        char c = '0'; send(_sockpair[0], &c, 1, 0); // safe
    }

}

std::string EventLoop::logSection()
{
    std::stringstream os;
    _postQueueLock.lock();
    MessageStack::size_type msgSize = _postQueue.size();
    _postQueueLock.unlock();
    os << " EventLoop: _epoll=" << _epoll << ", _sockpair[2]={" << _sockpair[0] << "," << _sockpair[1] << "}"
        << " _postQueue.size()=" << msgSize << ", current total timer=" << _timer.getTimersCount()
        << " _eventData=" << _eventData;
    return os.str();
}

void EventLoop::runOnce(bool isImmediately)
{
    int retCount = epoll_wait(_epoll, _events, MAX_EPOLL_WAIT,  isImmediately ? 0 : _timer.getNextExpireTime());
    if (retCount == -1)
    {
        if (errno != EINTR)
        {
            LOG(WARNING)<<"EventLoop::runOnce[this0x" << this << "]  epoll_wait err!  errno=" << strerror(errno) << logSection());
            return; //! error
        }
        return;
    }

    //check timer
    {
        _timer.checkTimer();
        if (retCount == 0) return;//timeout
    }


    for (int i=0; i<retCount; i++)
    {
        EventData * pEvent = (EventData *)_events[i].data.ptr;
        //tagHandle  type
        if (pEvent->_type == EventData::REG_ZSUMMER)
        {
            {
                char buf[200];
                while (recv(pEvent->_fd, buf, 200, 0) > 0);
            }

            MessageStack msgs;
            _postQueueLock.lock();
            msgs.swap(_postQueue);
            _postQueueLock.unlock();

            for (auto pfunc : msgs)
            {
                _OnPostHandler * p = (_OnPostHandler*)pfunc;
                try
                {
                    (*p)();
                }
                catch (const std::exception & e)
                {
                    LOG(WARNING)<<"OnPostHandler have runtime_error exception. err=" << e.what());
                }
                catch (...)
                {
                    LOG(WARNING)<<"OnPostHandler have unknown exception.");
                }
                delete p;
            }
        }
        else if (pEvent->_type == EventData::REG_TCP_ACCEPT)
        {
            if (pEvent->_tcpacceptPtr)
            {
                pEvent->_tcpacceptPtr->onEPOLLMessage(_events[i].events);
            }
        }
        else if (pEvent->_type == EventData::REG_TCP_SOCKET)
        {
            if (pEvent->_tcpSocketPtr)
            {
                pEvent->_tcpSocketPtr->onEPOLLMessage(_events[i].events);
            }
        }
        else
        {
            LOG(ERROR)<<"EventLoop::runOnce[this0x" << this << "] check register event type failed !!  type=" << pEvent->_type << logSection();
        }

    }
}

#endif
