#ifndef AGVTASK_H
#define AGVTASK_H
#include <memory>
#include <vector>
#include <atomic>
#include "agvtasknode.h"
#include "mapmap/agvline.h"
#include "agv.h"

class AgvTask;
using AgvTaskPtr = std::shared_ptr<AgvTask>;

//一个任务:由N个任务节点TaskNode助成
class AgvTask : public std::enable_shared_from_this<AgvTask>
{
public:
    /////////任务状态
    enum {
        AGV_TASK_STATUS_UNEXIST = -3,//不存在
        AGV_TASK_STATUS_UNEXCUTE = -2,//未执行
        AGV_TASK_STATUS_EXCUTING = -1,//正在执行
        AGV_TASK_STATUS_DONE = 0,//完成
        AGV_TASK_STATUS_FAIL = 1,//失败
        AGV_TASK_STATUS_CANCEL = 2//取消
    };

    ///////////任务优先级
    enum {
        PRIORITY_VERY_LOW = 0,//最低的优先级
        PRIORITY_LOW = 1,//低优先级
        PRIORITY_NORMAL = 2,//普通优先级
        PRIORITY_HIGH = 3,//高优先级
        PRIORITY_VERY_HIGH = 4,//最高优先级
        PRIORITY_LENGTH = 5,
    };

    AgvTask():
        id(0),
        excuteAgv(nullptr),
        status(AGV_TASK_STATUS_UNEXCUTE),
        priority(PRIORITY_NORMAL),
        error_code(0),
        isCancel(false)
    {
    }


    int getId(){return id;}
    void setId(int _id){id=_id;}

    std::string getProduceTime(){return produceTime;}
    void setProduceTime(std::string _produceTime){produceTime=_produceTime;}

    std::string getDoTime(){return doTime;}
    void setDoTime(std::string _doTime){doTime=_doTime;}

    std::string getDoneTime(){return doneTime;}
    void setDoneTime(std::string _doneTime){doneTime=_doneTime;}

    AgvPtr getAgv(){return excuteAgv;}
    void setAgv(AgvPtr agv){excuteAgv=agv;}

    std::vector<AgvTaskNodePtr> getTaskNode(){return nodes;}
    void setTaskNode(std::vector<AgvTaskNodePtr> _nodes){nodes=_nodes;}
    void push_backNode(AgvTaskNodePtr _node){nodes.push_back(_node);}

    int getDoingIndex(){return doingIndex;}
    void setDoingIndex(int _doingIndex){doingIndex = _doingIndex;}

    void setPath(std::vector<AgvLinePtr> _path){path = _path;}
    std::vector<AgvLinePtr> getPath(){return path;}

    int getPriority(){return priority;}
    void setPriority(int _priority){priority = _priority;}

    std::string getCancelTime(){return cancelTime;}
    void setCancelTime(std::string _cancelTime){cancelTime=_cancelTime;}

    std::string getErrorTime(){return errorTime;}
    std::string getErrorInfo(){return errorInfo;}
    int getErrorCode(){return error_code;}

    void setErrorTime(std::string _errorTime){errorTime = _errorTime;}
    void setErrorInfo(std::string _errorInfo){errorInfo = _errorInfo;}
    void setErrorCode(int code){error_code = code;}

    int getStatus(){return status;}
    void setStatus(int _status){status=_status;}

    void cancel(){isCancel = true;}
    bool getIsCancel(){return isCancel;}

private:
    std::vector<AgvTaskNodePtr> nodes;
    std::vector<AgvLinePtr> path;
    int id;
    std::string produceTime;
    std::string doTime;
    std::string doneTime;
    std::string cancelTime;//取消时间
    std::string errorTime;//发生错误时间
    std::string errorInfo;//发生错误的信息
    int error_code;//发生错误的代码
    AgvPtr excuteAgv;

    int status;//状态
    int priority;//优先级

    int doingIndex;

    std::atomic_bool isCancel;
};

#endif // AGVTASK_H
