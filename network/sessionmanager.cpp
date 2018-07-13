#include "sessionmanager.h"
#include "tcpsession.h"
#include "../msgprocess.h"
#include "../protocol.h"
#include "../common.h"

using namespace qyhnetwork;

namespace qyhnetwork {

SessionManager::SessionManager()
{
    memset(_statInfo, 0, sizeof(_statInfo));
    _summer = std::make_shared<EventLoop>();
}

bool SessionManager::start()
{
    if (!_summer->initialize())
    {
        return false;
    }
    _statInfo[STAT_STARTTIME] = time(NULL);
    return true;
}

void SessionManager::stop()
{
    _running = false;
}

void SessionManager::stopAccept(AccepterID aID)
{
    for (auto &ao : _mapAccepterOptions)
    {
        if (aID == InvalidAccepterID || ao.second._aID == aID)
        {
            if (!ao.second._closed)
            {
                ao.second._closed = true;
                if (ao.second._accepter)
                {
                    ao.second._accepter->close();
                }
            }
        }
    }
}


bool SessionManager::run()
{
    while (_running || !_mapTcpSessionPtr.empty())
    {
        runOnce(false);
    }
    return false;
}

bool SessionManager::runOnce(bool isImmediately)
{
    if (_running || !_mapTcpSessionPtr.empty())
    {
        _summer->runOnce(isImmediately);
        return true;
    }
    return false;
}


AccepterID SessionManager::addAccepter(const std::string & listenIP, unsigned short listenPort)
{
    _lastAcceptID = nextAccepterID(_lastAcceptID);
    auto & extend = _mapAccepterOptions[_lastAcceptID];
    extend._aID = _lastAcceptID;
    extend._listenIP = listenIP;
    extend._listenPort = listenPort;
    return _lastAcceptID;
}

AccepterOptions & SessionManager::getAccepterOptions(AccepterID aID)
{
    if (aID == InvalidAccepterID)
    {
        throw std::runtime_error(std::string("AccepterID can not be InvalidAccepterID"));
    }
    return _mapAccepterOptions[aID];
}

bool SessionManager::openAccepter(AccepterID aID)
{
    auto founder = _mapAccepterOptions.find(aID);
    if (founder == _mapAccepterOptions.end())
    {
        combined_logger->error("openAccepter error. not found the Accepter ID extend info. aID={0} ", aID);
        return false;
    }

    if (founder->second._accepter)
    {
        combined_logger->error("openAccepter error. already opened. ");
        return false;
    }
    if (founder->second._listenIP.empty())
    {
        combined_logger->warn("openAccepter warning. no listen IP. default use 0.0.0.0");
    }

    TcpAcceptPtr accepter = std::make_shared<TcpAccept>();
    if (!accepter->initialize(_summer))
    {
        combined_logger->error("openAccept error.");
        return false;
    }
    if (!accepter->openAccept(founder->second._listenIP, founder->second._listenPort, founder->second._setReuse))
    {
        combined_logger->error("openAccept error.");
        return false;
    }
    if (!accepter->doAccept(std::make_shared<qyhnetwork::TcpSocket>(),
                            std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, founder->second._aID)))
    {
        combined_logger->error("openAccept error.");
        return false;
    }
    combined_logger->info("openAccepter success. listenIP={0}, listenPort={1}", founder->second._listenIP.c_str(),founder->second._listenPort);
    founder->second._accepter = accepter;
    return true;
}


AccepterID SessionManager::getAccepterID(SessionID sID)
{
    if (!isSessionID(sID))
    {
        return InvalidAccepterID;
    }
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder == _mapTcpSessionPtr.end())
    {
        return InvalidAccepterID;
    }
    return founder->second->getAcceptID();
}


void SessionManager::onAcceptNewClient(qyhnetwork::NetErrorCode ec, const TcpSocketPtr& s, const TcpAcceptPtr &accepter, AccepterID aID)
{
    if (!_running)
    {
        combined_logger->info("onAcceptNewClient server already shutdown. accepter. aID={0}",aID);
        return;
    }
    auto founder = _mapAccepterOptions.find(aID);
    if (founder == _mapAccepterOptions.end())
    {
        combined_logger->error("onAcceptNewClient Unknown AccepterID aID={0}",aID);
        return;
    }
    if (founder->second._closed)
    {
        combined_logger->info("onAcceptNewClient accepter closed. accepter. aID={0}",aID);
        return;
    }
    if (ec)
    {
        combined_logger->error("onAcceptNewClient doAccept Result Error. ec={0}",ec );

        auto &&handler = std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID);
        auto timer = [accepter, handler]()
        {
            accepter->doAccept(std::make_shared<qyhnetwork::TcpSocket>(), std::move(handler));
        };
        createTimer(5000, std::move(timer));
        return;
    }

    std::string remoteIP;
    unsigned short remotePort = 0;
    s->getPeerInfo(remoteIP, remotePort);
    remoteIP = getPureHostName(remoteIP);
    //! check white list
    //! ---------------------
    if (!founder->second._whitelistIP.empty())
    {
        bool checkSucess = false;
        for (auto white : founder->second._whitelistIP)
        {
            if (remoteIP.size() >= white.size())
            {
                if (remoteIP.compare(0,white.size(), white) == 0)
                {
                    checkSucess = true;
                    break;
                }
            }
        }

        if (!checkSucess)
        {
            combined_logger->error("onAcceptNewClient Accept New Client Check Whitelist Failed remoteAdress={0}:{1}",remoteIP.c_str(),remotePort);
            accepter->doAccept(std::make_shared<qyhnetwork::TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
            return;
        }
        else
        {
            combined_logger->error("onAcceptNewClient Accept New Client Check Whitelist Success remoteAdress={0}:{1}",remoteIP.c_str(),remotePort);
        }
    }

    //! check Max Sessions
    if (founder->second._currentLinked >= founder->second._maxSessions)
    {
        combined_logger->error("onAcceptNewClient Accept New Client. Too Many Sessions And The new socket will closed") ;
    }
    else
    {

        founder->second._currentLinked++;
        founder->second._totalAcceptCount++;
        _lastSessionID = nextSessionID(_lastSessionID);

        combined_logger->info("onAcceptNewClient Accept New Client. Accept new Sessions sID={0}. The new socket  remoteAddress={1}:{2}, Already linked sessions = {3}",_lastSessionID,remoteIP.c_str(), remotePort,founder->second._currentLinked);

        s->initialize(_summer);

        TcpSessionPtr session = std::make_shared<qyhnetwork::TcpSession>();
        session->getOptions() = founder->second._sessionOptions;
        session->setEventLoop(_summer);
        _mapTcpSessionPtr[_lastSessionID] = session;
        if (!session->attatch(s, aID, _lastSessionID))
        {
            _mapTcpSessionPtr.erase(_lastSessionID);
        }
    }

    //! accept next socket.
    accepter->doAccept(std::make_shared<qyhnetwork::TcpSocket>(), std::bind(&SessionManager::onAcceptNewClient, this, std::placeholders::_1, std::placeholders::_2, accepter, aID));
}

std::string SessionManager::getRemoteIP(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder != _mapTcpSessionPtr.end())
    {
        return founder->second->getRemoteIP();
    }
    return "*";
}

unsigned short SessionManager::getRemotePort(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder != _mapTcpSessionPtr.end())
    {
        return founder->second->getRemotePort();
    }
    return -1;
}

void SessionManager::kickSessionByUserId(int userId)
{
    for (auto &ms : _mapTcpSessionPtr)
    {
        if (ms.second->getUserId() == userId)
        {
            ms.second->close();
        }
    }
}

void SessionManager::kickSession(SessionID sID)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end() || !iter->second)
    {
        combined_logger->info("kickSession NOT FOUND SessionID. SessionID={0}", sID);
        return;
    }
    combined_logger->info("kickSession SessionID. SessionID={0}",sID);
    iter->second->close();
}

void SessionManager::kickClientSession(AccepterID aID)
{
    for (auto &ms : _mapTcpSessionPtr)
    {
        if (aID == InvalidAccepterID || ms.second->getAcceptID() == aID)
        {
            ms.second->close();
        }
    }
}
void SessionManager::kickConnect(SessionID cID)
{
    if (cID != InvalidSessionID)
    {
        auto iter = _mapTcpSessionPtr.find(cID);
        if (iter == _mapTcpSessionPtr.end() || !iter->second)
        {
            combined_logger->warn("SessionManager::kickConnect NOT FOUND SessionID. SessionID={0}",cID);
            return;
        }
        iter->second->getOptions()._reconnects = 0;
        iter->second->close();
        combined_logger->info("SessionManager::kickConnect cID={0}",cID);
    }
    else
    {
        for (auto &ms : _mapTcpSessionPtr)
        {
            if ( ms.second->getAcceptID() == InvalidAccepterID)
            {
                ms.second->getOptions()._reconnects = 0;
                ms.second->close();
                combined_logger->info("SessionManager::kickConnect [all] cID={0}",ms.second->getSessionID());
            }
        }
    }
}

void SessionManager::removeSession(TcpSessionPtr session)
{
	
	//通知订阅信息，取消该session的所有订阅
    MsgProcess::getInstance()->removeSubSession(session->getSessionID());
	
	//设置登录状态
	MsgProcess::getInstance()->sessionLogout(session->getUserId());
    
	//删除session
	_mapTcpSessionPtr.erase(session->getSessionID());
    if (session->getAcceptID() != InvalidAccepterID)
    {
        _mapAccepterOptions[session->getAcceptID()]._currentLinked--;
        _mapAccepterOptions[session->getAcceptID()]._totalAcceptCount++;
    }
}

SessionID SessionManager::addConnecter(const std::string & remoteHost, unsigned short remotePort)
{
    std::string remoteIP = getHostByName(remoteHost);
    if (remoteIP.empty())
    {
        return InvalidSessionID;
    }
    _lastConnectID = nextConnectID(_lastConnectID);
    TcpSessionPtr & session = _mapTcpSessionPtr[_lastConnectID];
    session = std::make_shared<qyhnetwork::TcpSession>();

    session->setEventLoop(_summer);
    session->setSessionID(_lastConnectID);
    session->setRemoteIP(remoteIP);
    session->setRemotePort(remotePort);
    return _lastConnectID;
}

SessionOptions & SessionManager::getConnecterOptions(SessionID cID)
{
    auto founder = _mapTcpSessionPtr.find(cID);
    if (founder == _mapTcpSessionPtr.end())
    {
        throw std::runtime_error("getConnecterOptions error.");
    }
    return founder->second->getOptions();
}

bool SessionManager::openConnecter(SessionID cID)
{
    auto founder = _mapTcpSessionPtr.find(cID);
    if (founder == _mapTcpSessionPtr.end())
    {
        combined_logger->error("openConnecter error");
        return false;
    }
    founder->second->connect();
    return true;
}

TcpSessionPtr SessionManager::getTcpSession(SessionID sID)
{
    auto founder = _mapTcpSessionPtr.find(sID);
    if (founder != _mapTcpSessionPtr.end())
    {
        return founder->second;
    }
    return nullptr;
}



void SessionManager::sendSessionData(SessionID sID, const Json::Value &response)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end())
    {
        combined_logger->warn("sendSessionData NOT FOUND SessionID.  SessionID={0}",sID);
        return;
    }
    iter->second->send(response);
}

void SessionManager::sendSessionData(SessionID sID, const char *data, int len)
{
    auto iter = _mapTcpSessionPtr.find(sID);
    if (iter == _mapTcpSessionPtr.end())
    {
        combined_logger->warn("sendSessionData NOT FOUND SessionID.  SessionID={0}",sID);
        return;
    }
    iter->second->send(data);
}


void SessionManager::sendData(const Json::Value &response)
{
    for (auto &ms : _mapTcpSessionPtr)
    {
        ms.second->send(response);
    }
}

}

