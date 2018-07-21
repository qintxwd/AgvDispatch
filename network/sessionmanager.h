#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H
#include <memory>
#include "networkconfig.h"
#include "tcpsession.h"

#include "../utils/noncopyable.h"

namespace qyhnetwork
{

class SessionManager;
using SessionManagerPtr = std::shared_ptr<SessionManager>;

class SessionManager : public noncopyable,public std::enable_shared_from_this<SessionManager>
{
private:
    SessionManager();

public:
    static SessionManagerPtr getInstance(){
        static SessionManagerPtr m_inst = SessionManagerPtr(new SessionManager());
        return m_inst;
    }
public:
    //要使用SessionManager必须先调用start来启动服务.
    bool start();

    //退出消息循环.
    void stop();

    //阻塞当前线程并开始消息循环. 默认选用这个比较好. 当希望有更细力度的控制run的时候推荐使用runOnce
    bool run();
    inline bool isRunning(){ return _running; }

    //执行一次消息处理, 如果isImmediately为true, 则无论当前处理有无数据 都需要立即返回, 可以嵌入到任意一个线程中灵活使用
    //默认为false,  如果没有网络消息和事件消息 则会阻塞一小段时间, 有消息通知会立刻被唤醒.
    bool runOnce(bool isImmediately = false);

    //handle: std::function<void()>
    //switch initiative, in the multi-thread it's switch call thread simultaneously.
    //投递一个handler到SessionManager的线程中去处理, 线程安全.
    template<class H>
    void post(H &&h){ _summer->post(std::move(h)); }

    //it's blocking call. support ipv6 & ipv4 .
    inline std::string getHostByName(const std::string & name) { return qyhnetwork::getHostByName(name); }
    //创建定时器 单位是毫秒 非线程安全.
    template <class H>
    qyhnetwork::TimerID createTimer(unsigned int delayms, H &&h, bool useSystemTime = true)
    { return _summer->createTimer(delayms, std::move(h), useSystemTime); }
    template <class H>
    qyhnetwork::TimerID createTimer(unsigned int delayms, const H &h, bool useSystemTime = true)
    {
        return _summer->createTimer(delayms, h, useSystemTime);
    }
    //取消定时器.  注意, 如果在定时器的回调handler中取消当前定时器 会失败的.
    bool cancelTimer(unsigned long long timerID){ return _summer->cancelTimer(timerID); }

    AccepterID addAccepter(const std::string& listenIP, unsigned short listenPort);
    AccepterOptions & getAccepterOptions(AccepterID aID);
    bool openAccepter(AccepterID aID);
    AccepterID getAccepterID(SessionID sID);


    SessionID addConnecter(const std::string& remoteHost, unsigned short remotePort);
    SessionOptions & getConnecterOptions(SessionID cID);
    bool openConnecter(SessionID cID);
    TcpSessionPtr getTcpSession(SessionID sID);

    std::string getRemoteIP(SessionID sID);
    unsigned short getRemotePort(SessionID sID);

    //send data.
    void sendSessionData(SessionID sID, const Json::Value &response);
    //send data
    void sendSessionData(SessionID sID, const char *data, int len);
    //send data to all session
    void sendData(const Json::Value &response);

    //close session socket.
    void kickSessionByUserId(int userId);
    void kickSession(SessionID sID);
    void kickClientSession(AccepterID aID = InvalidAccepterID);
    void kickConnect(SessionID cID = InvalidSessionID);
    void stopAccept(AccepterID aID = InvalidAccepterID);
public:
    //statistical information
    inline unsigned long long getStatInfo(int stat){ return _statInfo[stat]; }
    alignas(unsigned long long) unsigned long long _statInfo[STAT_SIZE];

private:
    friend class TcpSession;
    // 一个established状态的session已经关闭.
    void removeSession(TcpSessionPtr session);

    //accept到新连接.
    void onAcceptNewClient(qyhnetwork::NetErrorCode ec, const TcpSocketPtr & s, const TcpAcceptPtr & accepter, AccepterID aID);
private:
    //消息循环
    EventLoopPtr _summer;

    //! 以下一组参数均为控制消息循环的开启和关闭用的
    bool  _running = true;  //默认是开启, 否则会在合适的时候退出消息循环.

    //!以下一组ID用于生成对应的unique ID.
    AccepterID _lastAcceptID = InvalidAccepterID; //accept ID sequence. range  (0 - power(2,32))
    SessionID _lastSessionID = InvalidSessionID;//session ID sequence. range  (0 - __MIDDLE_SEGMENT_VALUE)
    SessionID _lastConnectID = InvalidSessionID;//connect ID sequence. range  (__MIDDLE_SEGMENT_VALUE - power(2,32))

    //!存储当前的连入连出的session信息和accept监听器信息.
    std::unordered_map<SessionID, TcpSessionPtr> _mapTcpSessionPtr;
    std::unordered_map<AccepterID, AccepterOptions > _mapAccepterOptions;
};

}


#endif // SESSIONMANAGER_H
