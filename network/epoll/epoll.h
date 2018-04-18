#ifndef EPOLL_H
#define EPOLL_H

#include "epoll_common.h"
#include "../timer/timer.h"
#ifndef WIN32

namespace qyhnetwork{

class EventLoop : public std::enable_shared_from_this<EventLoop>
{
public:
    using MessageStack = std::vector<void*>;
    EventLoop(){}
    ~EventLoop(){}
    bool initialize();
    void runOnce(bool isImmediately = false);

    template <typename handle>
    inline void post(handle &&h){PostMessage(std::move(h));}
    inline unsigned long long createTimer(unsigned int delayms, _OnTimerHandler &&handle, bool useSystemTime = true)
    {
        return _timer.createTimer(delayms, std::move(handle), useSystemTime);
    }
    inline unsigned long long createTimer(unsigned int delayms, const _OnTimerHandler &handle, bool useSystemTime = true)
    {
        return _timer.createTimer(delayms, handle, useSystemTime);
    }
    inline bool cancelTimer(unsigned long long timerID)
    {
        return _timer.cancelTimer(timerID);
    }
public:
    bool registerEvent(int op, EventData &ed);
    void PostMessage(_OnPostHandler &&handle);
private:
    std::string logSection();
private:
    int    _epoll = InvalidFD;
    epoll_event _events[MAX_EPOLL_WAIT] = {};
    int        _sockpair[2] = {};
    EventData _eventData;
    MessageStack _postQueue;
    std::mutex     _postQueueLock;
    Timer _timer;
};

using EventLoopPtr = std::shared_ptr<EventLoop>;

}
#endif


#endif // EPOLL_H
