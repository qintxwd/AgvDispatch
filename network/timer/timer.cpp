#include  "timer.h"
#include <algorithm>

#include "../../utils/Log/spdlog/spdlog.h"
#include "../../common.h"

using namespace qyhnetwork;


Timer::Timer()
{

}
Timer::~Timer()
{
}
//get current time tick. unit is millisecond.
unsigned long long Timer::getSteadyTick()
{
    return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

//get current time tick. unit is millisecond.
unsigned long long Timer::getSystemTick()
{
    return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

TimerID Timer::makeTimeID(bool useSystemTick, unsigned int delayTick)
{
    unsigned long long tick = 0;
    if (useSystemTick)
    {
        tick = getSystemTick() - _startSystemTime;
    }
    else
    {
        tick = getSteadyTick() - _startSteadyTime;
    }
    tick += delayTick;
    tick <<= SequenceBit;
    tick |= (++_queSeq & SequenceMask);
    tick &= TimeSeqMask;
    if (useSystemTick)
    {
        tick |= UsedSysMask;
    }
    return tick;
}
std::pair<bool, unsigned long long> Timer::resolveTimeID(TimerID timerID)
{
    return std::make_pair((timerID & UsedSysMask) != 0, (timerID & TimeSeqMask) >> SequenceBit);
}
using std::min;
//get next expire time  be used to set timeout when calling select / epoll_wait / GetQueuedCompletionStatus.
unsigned int Timer::getNextExpireTime()
{
    unsigned long long steady = -1;
    unsigned long long sys = -1;
    if (!_sysQue.empty())
    {
        sys = getSystemTick() - _startSystemTime - resolveTimeID(_sysQue.begin()->first).second;
    }
    if (!_steadyQue.empty())
    {
        steady = getSteadyTick() - _startSteadyTime - resolveTimeID(_steadyQue.begin()->first).second;
    }
    return (unsigned int)min(min(sys, steady), 100ULL);
}
TimerID Timer::createTimer(unsigned int delayTick, const _OnTimerHandler &handle, bool useSysTime)
{
    return createTimer(delayTick, std::bind(handle), useSysTime);
}
TimerID Timer::createTimer(unsigned int delayTick, _OnTimerHandler &&handle, bool useSysTime)
{
    _OnTimerHandler *pfunc = new _OnTimerHandler(std::move(handle));
    TimerID timerID = makeTimeID(useSysTime, delayTick);
    if (useSysTime)
    {
        while (!_sysQue.insert(std::make_pair(timerID, pfunc)).second)
        {
            timerID = makeTimeID(useSysTime, delayTick);
        }
    }
    else
    {
        while (!_steadyQue.insert(std::make_pair(timerID, pfunc)).second)
        {
            timerID = makeTimeID(useSysTime, delayTick);
        }
    }
    return timerID;
}

bool Timer::cancelTimer(TimerID timerID)
{
    auto ret = resolveTimeID(timerID);
    if (ret.first)
    {
        auto founder = _sysQue.find(timerID);
        if (founder != _sysQue.end())
        {
            delete founder->second;
            _sysQue.erase(founder);
            return true;
        }
    }
    else
    {
        auto founder = _steadyQue.find(timerID);
        if (founder != _steadyQue.end())
        {
            delete founder->second;
            _steadyQue.erase(founder);
            return true;
        }
    }
    return false;
}

void Timer::checkTimer()
{
    if (!_steadyQue.empty())
    {
        unsigned long long now = getSteadyTick() - _startSteadyTime;
        while (!_steadyQue.empty() && now > resolveTimeID(_steadyQue.begin()->first).second)
        {
            TimerID timerID = _steadyQue.begin()->first;
            _OnTimerHandler * handler = _steadyQue.begin()->second;
            //erase the pointer from timer queue before call handler.
            _steadyQue.erase(_steadyQue.begin());
            try
            {
                //combined_logger->info()<<"call timer(). now=" << (now >> ReserveBit)  << ", expire=" << (timerID >> ReserveBit));
                (*handler)();
            }
            catch (const std::exception & e)
            {
                //combined_logger->warn("OnTimerHandler have runtime_error exception. timerID={0}, err={1}",timerID,e.what());
            }
            catch (...)
            {
                //combined_logger->warn("OnTimerHandler have unknown exception. timerID={0}",timerID);
            }
            delete handler;
        }
    }
    if (!_sysQue.empty())
    {
        unsigned long long now = getSystemTick() - _startSystemTime;
        while (!_sysQue.empty() && now > resolveTimeID(_sysQue.begin()->first).second)
        {
            TimerID timerID = _sysQue.begin()->first;
            _OnTimerHandler * handler = _sysQue.begin()->second;
            //erase the pointer from timer queue before call handler.
            _sysQue.erase(_sysQue.begin());
            try
            {
                //combined_logger->info()<<"call timer(). now=" << (now >> ReserveBit)  << ", expire=" << (timerID >> ReserveBit));
                (*handler)();
            }
            catch (const std::exception & e)
            {
                //combined_logger->warn("OnTimerHandler have runtime_error exception. timerID={0}, err={1}", timerID,e.what());
            }
            catch (...)
            {
                //combined_logger->warn("OnTimerHandler have unknown exception. timerID={0}" , timerID);
            }
            delete handler;
        }
    }
}

