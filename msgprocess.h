#ifndef MSGPROCESS_H
#define MSGPROCESS_H
#include "utils/noncopyable.h"
#include "Protocol.h"
#include "network/networkconfig.h"

class MsgProcess;

using MsgProcessPtr = std::shared_ptr<MsgProcess>;

typedef enum{
    ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE = 0,
    ENUM_NOTIFY_ALL_TYPE_ERROR,
}ENUM_NOTIFY_ALL_TYPE;

class MsgProcess : public noncopyable,public std::enable_shared_from_this<MsgProcess>
{
public:

    static MsgProcessPtr getInstance(){
        static MsgProcessPtr m_inst = MsgProcessPtr(new MsgProcess());
        return m_inst;
    }

    bool init();

    void removeSubSession(int session);

    //进来一个消息,分配给一个线程去处理它
    void processOneMsg(MSG_Request request,qyhnetwork::TcpSessionPtr session);

    //发布一个日志消息
    void publishOneLog(USER_LOG log);

    //通知所有用户的事件
    void notifyAll(ENUM_NOTIFY_ALL_TYPE type);

    //发生错误，需要告知
    void errorOccur(int code,std::string msg,bool needConfirm);

    //用户接口
    void interAddSubAgvPosition(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interAddSubAgvStatus(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interAddSubTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interAddSubLog(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interRemoveSubAgvPosition(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interRemoveSubAgvStatus(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interRemoveSubTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interRemoveSubLog(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);

    void addSubAgvPosition(int id);
    void addSubAgvStatus(int id);
    void addSubTask(int id);
    void addSubLog(int id);

    void removeSubAgvPosition(int id);
    void removeSubAgvStatus(int id);
    void removeSubTask(int id);
    void removeSubLog(int id);
private:

    void publisher_agv_position();

    void publisher_agv_status();

    void publisher_task();

    void publisher_log();

    MsgProcess();

    std::mutex psMtx;
    std::list<int> agvPositionSubers;

    std::mutex ssMtx;
    std::list<int> agvStatusSubers;

    std::mutex tsMtx;
    std::list<int> taskSubers;

    std::mutex lsMtx;
    std::list<int> logSubers;

    std::mutex errorMtx;
    int error_code;
    std::string error_info;
    bool needConfirm;
};



#endif // MSGPROCESS_H
