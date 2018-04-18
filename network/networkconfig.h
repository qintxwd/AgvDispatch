#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

#include <iostream>
#include <queue>
#include <iomanip>
#include <string.h>
#include <signal.h>
#include <unordered_map>
#include "../utils/Log/easylogging.h"

#ifdef WIN32
#pragma warning(disable:4503)
#pragma warning(disable:4200)
#include "iocp/iocp.h"
#include "iocp/iocpsocket.h"
#include "iocp/iocpaccept.h"
#include "iocp/iocp_common.h"
#else
#include "epoll/epoll.h"
#include "epoll/epollsocket.h"
#include "epoll/epollaccept.h"
#endif


namespace qyhnetwork
{
//! define network type
using SessionID = unsigned int;
const SessionID InvalidSessionID = 0;
using AccepterID = unsigned int;
const AccepterID InvalidAccepterID = 0;


//! define session id range.
const unsigned int __MIDDLE_SEGMENT_VALUE = 300 * 1000 * 1000;
//(InvalidSessionID - __MIDDLE_SEGMENT_VALUE)
inline bool isSessionID(unsigned int unknowID){ return unknowID != InvalidSessionID && unknowID < __MIDDLE_SEGMENT_VALUE; }
//(InvalidSessionID - power(2, 32))
inline bool isConnectID(unsigned int unknowID){ return unknowID != InvalidSessionID && unknowID >= __MIDDLE_SEGMENT_VALUE; }
inline SessionID nextSessionID(SessionID curSessionID){ return curSessionID + 1 >= __MIDDLE_SEGMENT_VALUE ? 1 : curSessionID + 1; }
inline SessionID nextConnectID(SessionID curSessionID){ return curSessionID + 1 <= __MIDDLE_SEGMENT_VALUE ? __MIDDLE_SEGMENT_VALUE +1 : curSessionID + 1; }
inline AccepterID nextAccepterID(AccepterID AccepterID){ return AccepterID + 1 >= __MIDDLE_SEGMENT_VALUE ? 1 : AccepterID + 1; }

enum StatType
{
    STAT_STARTTIME,
    STAT_SESSION_CREATED,
    STAT_SESSION_DESTROYED,
    STAT_SESSION_LINKED,
    STAT_SESSION_CLOSED,
    STAT_FREE_BLOCKS,
    STAT_EXIST_BLOCKS,
    STAT_SEND_COUNT,
    STAT_SEND_PACKS,
    STAT_SEND_BYTES,
    STAT_SEND_QUES,
    STAT_RECV_COUNT,
    STAT_RECV_PACKS,
    STAT_RECV_BYTES,

    STAT_SIZE,
};
const char * const StatTypeDesc[] = {
    "STAT_STARTTIME",
    "STAT_SESSION_CREATED",
    "STAT_SESSION_DESTROYED",
    "STAT_SESSION_LINKED",
    "STAT_SESSION_CLOSED",
    "STAT_FREE_BLOCKS",
    "STAT_EXIST_BLOCKS",
    "STAT_SEND_COUNT",
    "STAT_SEND_PACKS",
    "STAT_SEND_BYTES",
    "STAT_SEND_QUES",
    "STAT_RECV_COUNT",
    "STAT_RECV_PACKS",
    "STAT_RECV_BYTES",
    "STAT_SIZE",
};

class TcpSession;
using TcpSessionPtr = std::shared_ptr<TcpSession>;

using OnSessionEvent = std::function<void(const TcpSessionPtr &  /*session*/)>;

struct SessionOptions: public el::Loggable
{
    bool            _setNoDelay = true;
    unsigned int    _sessionPulseInterval = 30000;
    unsigned int    _connectPulseInterval = 5000;
    unsigned int    _reconnects = 0; // can reconnect count
    unsigned int    _maxSendListCount = 600;
    OnSessionEvent _onReconnectEnd;
    OnSessionEvent _onSessionClosed;
    OnSessionEvent _onSessionLinked;
    OnSessionEvent _onSessionPulse;

    virtual void log(el::base::type::ostream_t& os) const{
        os << "{ _setNoDelay=" << _setNoDelay
           << ", _sessionPulseInterval=" << _sessionPulseInterval
           << ", _connectPulseInterval=" << _connectPulseInterval
           << ", _reconnects=" << _reconnects
           << "}";
    }

};

struct AccepterOptions : public el::Loggable
{
    AccepterID _aID = InvalidAccepterID;
    TcpAcceptPtr _accepter;
    std::string        _listenIP;
    unsigned short    _listenPort = 0;
    bool            _setReuse = true;
    unsigned int    _maxSessions = 5000;
    std::vector<std::string> _whitelistIP;
    unsigned long long _totalAcceptCount = 0;
    unsigned long long _currentLinked = 0;
    bool _closed = false;
    SessionOptions _sessionOptions;
    virtual void log(el::base::type::ostream_t& os) const
    {
        os << "{"
           << "_aID=" << _aID
           << ", _listenIP=" << _listenIP
           << ", _listenPort=" << _listenPort
           << ", _maxSessions=" << _maxSessions
           << ",_totalAcceptCount=" << _totalAcceptCount
           << ", _currentLinked=" << _currentLinked
           << ",_whitelistIP={";

        for (auto str : _whitelistIP)
        {
            os << str << ",";
        }
        os << "}";
        os << ", SessionOptions=" << _sessionOptions;
        os << "}";
    }
};

}

#endif
