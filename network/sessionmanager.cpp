#include "sessionmanager.h"
#include "../common.h"
#include "tcpsession.h"
#include "tcpacceptor.h"
#include "websocketsession.h"
#include "websocketacceptor.h"

#include "../msgprocess.h"

SessionManager::SessionManager():
    nextAcceptId(0),
    nextSessionId(0)
{
}

int SessionManager::addTcpAccepter(int port)
{
    TcpAcceptorPtr acceptor = std::make_shared<TcpAcceptor>(io_context,port,++nextAcceptId);
    _mapAcceptorPtr[acceptor->getId()] = acceptor;
    return acceptor->getId();
}

void SessionManager::openTcpAccepter(int aID)
{
    for(auto a = _mapAcceptorPtr.begin();a!=_mapAcceptorPtr.end();++a){
        if(a->first == aID ){
            a->second->start();
            break;
        }
    }
}

int SessionManager::addWebSocketAccepter(int port)
{
    WebsocketAcceptorPtr acceptor = std::make_shared<WebsocketAcceptor>(io_context, port, ++nextAcceptId);
    _mapAcceptorPtr[acceptor->getId()] = acceptor;
    return acceptor->getId();
}
void SessionManager::openWebSocketAccepter(int aID)
{
    for (auto a = _mapAcceptorPtr.begin(); a != _mapAcceptorPtr.end(); ++a) {
        if (a->first == aID) {
            a->second->start();
            break;
        }
    }
}

void SessionManager::run()
{
    io_context.run();
}

void SessionManager::kickSession(int sID)
{
    auto iter = _mapSessionPtr.find(sID);
    if (iter == _mapSessionPtr.end())
    {
        combined_logger->info("kickSession NOT FOUND SessionID. SessionID={0}", sID);
        return;
    }
    combined_logger->info("kickSession SessionID. SessionID={0}",sID);
    iter->second->close();
}

void SessionManager::sendSessionData(int sID, const Json::Value &response)
{
    auto iter = _mapSessionPtr.find(sID);
    if (iter == _mapSessionPtr.end())
    {
        combined_logger->warn("sendSessionData NOT FOUND SessionID.  SessionID={0}",sID);
        return;
    }
    iter->second->send(response);
}

void SessionManager::sendData(const Json::Value &response)
{
    for (auto &ms : _mapSessionPtr)
    {
        ms.second->send(response);
    }
}

void SessionManager::kickSessionByUserId(int userId)
{
    for (auto &ms : _mapSessionPtr)
    {
        if (ms.second->getUserId() == userId)
        {
            ms.second->close();
        }
    }
}

void SessionManager::addSession(int id,SessionPtr session)
{
    _mapSessionPtr[id] = session;
}
void SessionManager::removeSession(SessionPtr session)
{
    _mapSessionPtr.erase(session->getSessionID());
    //取消该session的所有订阅
    MsgProcess::getInstance()->onSessionClosed(session->getSessionID());
}
