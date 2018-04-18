#ifndef MSGPROCESS_H
#define MSGPROCESS_H

#include "utils/noncopyable.h"
#include "threadpool.h"
#include "Protocol.h"
#include "network/networkconfig.h"

class MsgProcess : public noncopyable
{
public:

    static MsgProcess* getInstance(){
        return p;
    }

    bool init();


    //进来一个消息,分配给一个线程去处理它
    void processOneMsg(MSG_Request request,qyhnetwork::TcpSessionPtr session);

    void publishOneLog(USER_LOG log);
private:

    void publisher_agv_position();

    void publisher_agv_status();

    void publisher_task();

    void publisher_log();

    static MsgProcess* p;

    MsgProcess();

    ThreadPool pool;

    std::list<qyhnetwork::TcpSessionPtr> agvPositionSubers;
    std::list<qyhnetwork::TcpSessionPtr> agvStatusSubers;
    std::list<qyhnetwork::TcpSessionPtr> taskSubers;
    std::list<qyhnetwork::TcpSessionPtr> logSubers;
};

#endif // MSGPROCESS_H
