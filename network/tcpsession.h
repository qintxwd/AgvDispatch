#ifndef TCPSESSION_H
#define TCPSESSION_H

#include "networkconfig.h"
#include "../protocol.h"
#ifdef WIN32
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif
#include <fstream>

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
    void send(const Json::Value &json);
    void close();

private:

    bool doRecv();
    unsigned int onRecv(qyhnetwork::NetErrorCode ec, int received);
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
    inline qyhnetwork::NetErrorCode getLastError(){ return _lastRecvError; }
    inline void setUserId(int _user_id){user_id = _user_id;}
    inline int getUserId(){return user_id;}
    inline void setUserRole(int _user_role){user_role = _user_role;}
    inline int getUserRole(){return user_role;}
    inline void setUserName(std::string _user_name){username = _user_name;}
    inline std::string getUserName(){return username;}
private:
    SessionOptions _options;
    EventLoopPtr _eventLoop;
    TcpSocketPtr  _sockptr;
    std::string _remoteIP;
    unsigned short _remotePort = 0;
    int _status = 0; //0 uninit, 1 connecting, 2 session established, 3  died

    SessionID _sessionID = InvalidSessionID;
    AccepterID _acceptID = InvalidAccepterID;
    qyhnetwork::TimerID _pulseTimerID = qyhnetwork::InvalidTimerID;

    //读取数据缓存区[一个小的消息就直接处理了，过长的消息，放入big_msg_buffer中]
    char read_buffer[MSG_READ_BUFFER_LENGTH];

    //读取的长度
    int read_len;

    //要读取的json的数据长度
    int json_len;

    int read_position;

    bool isContinueRecving;//持续接收,json过长时，写入big_msg_buffer中，等待接收完成

    unsigned long long _reconnects = 0;

    qyhnetwork::NetErrorCode _lastRecvError = NEC_SUCCESS;

    //连接保存一个用户的两个信息
    int user_id = 0;
    int user_role = 0;
    std::string username;

	char *big_msg_buffer;//
	int big_msg_buffer_position;
};
using TcpSessionPtr = std::shared_ptr<TcpSession>;

}



#endif // TCPSESSION_H
