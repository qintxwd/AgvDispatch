#ifndef TCPSESSION_H
#define TCPSESSION_H

#include "networkconfig.h"

namespace qyhnetwork{

class TcpSession: public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession();
    ~TcpSession();
    inline SessionOptions & getOptions(){ return _options; }
    void connect();
    void reconnect();
    bool attatch(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID);
    void send(const char *buf, unsigned int len);
    void close();

private:

    bool doRecv();
    unsigned int onRecv(qyhnetwork::NetErrorCode ec, int received);
    void onSend(qyhnetwork::NetErrorCode ec, int sent);
    void onConnected(qyhnetwork::NetErrorCode ec);
    void onPulse();

public:
    inline void setEventLoop(EventLoopPtr el){ _eventLoop = el; }
    inline SessionID getAcceptID(){ return _acceptID; }
    inline SessionID getSessionID(){ return _sessionID; }
    inline void setSessionID(SessionID sID){ _sessionID = sID; }
    inline bool isInvalidSession(){ return !_sockptr; }
    inline const std::string & getRemoteIP(){ return _remoteIP; }
    inline void setRemoteIP(const std::string &remoteIP){ _remoteIP = remoteIP; }
    inline unsigned short getRemotePort(){ return _remotePort; }
    inline void setRemotePort(unsigned short remotePort){ _remotePort = remotePort; }
    inline std::size_t getSendQueSize(){ return _sendque.size(); }
    inline qyhnetwork::NetErrorCode getLastError(){ return _lastRecvError; }

    inline const TupleParam& getUserParam(size_t index) { return peekTupleParamImpl(index); }
    inline void setUserParam(size_t index, const TupleParam &tp) { autoTupleParamImpl(index) = tp; }
    inline bool isUserParamInited(size_t index) { return std::get<TupleParamInited>(peekTupleParamImpl(index)); }
    inline double getUserParamNumber(size_t index) { return std::get<TupleParamNumber>(peekTupleParamImpl(index)); }
    inline void setUserParamNumber(size_t index, double f) { std::get<TupleParamNumber>(autoTupleParamImpl(index)) = f; }
    inline unsigned long long getUserParamInteger(size_t index) { return std::get<TupleParamInteger>(peekTupleParamImpl(index)); }
    inline void setUserParamInteger(size_t index, unsigned long long ull) { std::get<TupleParamInteger>(autoTupleParamImpl(index)) = ull; }
    inline const std::string & getUserParamString(size_t index) { return std::get<TupleParamString>(peekTupleParamImpl(index)); }
    inline void setUserParamString(size_t index, const std::string & str) { std::get<TupleParamString>(autoTupleParamImpl(index)) = str; }

private:
    SessionOptions _options;
    EventLoopPtr _eventLoop;
    TcpSocketPtr  _sockptr;
    std::string _remoteIP;
    unsigned short _remotePort = 0;
    int _status = 0; //0 uninit, 1 connecting, 2 session established, 3  died

    //
    SessionID _sessionID = InvalidSessionID;
    AccepterID _acceptID = InvalidAccepterID;
    qyhnetwork::TimerID _pulseTimerID = qyhnetwork::InvalidTimerID;

    //!
    SessionBlock* _recving = nullptr;
    SessionBlock* _sending = nullptr;
    unsigned int _sendingLen = 0;

    //! send data queue
    std::deque<SessionBlock *> _sendque;
    unsigned long long _reconnects = 0;

    //!
    bool _bFirstRecvData = true;

    //!last recv error code
    qyhnetwork::NetErrorCode _lastRecvError = NEC_SUCCESS;

    //! http status data
    bool _httpIsChunked = false;
    std::string _httpMethod;
    std::string _httpMethodLine;
    std::map<std::string, std::string> _httpHeader;


    //! user param
    std::vector<TupleParam> _param;
    TupleParam & autoTupleParamImpl(size_t index);
    const TupleParam & peekTupleParamImpl(size_t index) const;

};
using TcpSessionPtr = std::shared_ptr<TcpSession>;

}



#endif // TCPSESSION_H
