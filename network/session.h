#ifndef SESSION_H
#define SESSION_H

#include <memory>

#ifdef WIN32
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif

class Session;
using SessionPtr = std::shared_ptr<Session>;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(int sessionId,int acceptId);

    virtual ~Session();

    virtual void send(const Json::Value &json) = 0;

    virtual bool doSend(const char *data,int len) = 0;

    virtual void close() = 0;

    virtual void start() = 0;

    int getSessionID(){ return _sessionID; }

    void setSessionID(int sID){ _sessionID = sID; }

    int getAcceptID(){ return _acceptID; }

    void setAcceptnID(int sID){ _acceptID = sID; }

    void setUserId(int _user_id){user_id = _user_id;}

    int getUserId(){return user_id;}

    void setUserRole(int _user_role){user_role = _user_role;}

    int getUserRole(){return user_role;}

    void setUserName(std::string _user_name){username = _user_name;}

    std::string getUserName(){return username;}

protected:
    int _sessionID = -1;
    int _acceptID = -1;

    //连接保存一个用户的两个信息
    int user_id = 0;
    int user_role = 0;
    std::string username;

};

#endif // SESSION_H
