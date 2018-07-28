#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <deque>
#include <iostream>
#include <list>
#include <unordered_map>
#include <memory>
#include <set>
#include <utility>
#include <atomic>
#include <boost/asio.hpp>
#include "../utils/noncopyable.h"
#include "session.h"
#include "acceptor.h"


using boost::asio::ip::tcp;

class SessionManager;
using SessionManagerPtr = std::shared_ptr<SessionManager>;

class SessionManager: public noncopyable,public std::enable_shared_from_this<SessionManager>
{
public:
    static SessionManagerPtr getInstance(){
        static SessionManagerPtr m_inst = SessionManagerPtr(new SessionManager());
        return m_inst;
    }

    int addTcpAccepter(int port);
    void openTcpAccepter(int aID);

    int addWebSocketAccepter(int port);
    void openWebSocketAccepter(int aID);

    void run();
    void kickSession(int sID);
    void kickSessionByUserId(int userId);

    void sendSessionData(int sID, const Json::Value &response);
    void sendData(const Json::Value &response);
    int getNextSessionId(){return ++nextSessionId;}

    void addSession(int id,SessionPtr session);
    void removeSession(SessionPtr session);
private:
    std::atomic_int nextAcceptId;
    std::atomic_int nextSessionId;

    boost::asio::io_context io_context;

    SessionManager();

    std::unordered_map<int, SessionPtr> _mapSessionPtr;
    std::unordered_map<int, AcceptorPtr > _mapAcceptorPtr;
};

#endif // SESSIONMANAGER_H
