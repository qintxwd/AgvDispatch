#ifndef TASKMAKER_H
#define TASKMAKER_H

#include "network/session.h"
#include "protocol.h"
#include "agvtask.h"

class TaskMaker
{
public:
    static  TaskMaker* getInstance();
    virtual ~TaskMaker();

    virtual void init() = 0;

    virtual void makeTask(SessionPtr conn, const Json::Value &request) = 0;

    //群创接口//非群创不需要重写
    virtual void makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info);
    virtual bool cancelTask(std::string dispatch_id, int agv_id) {return true;} //取消任务
    virtual bool forceFinishTask(std::string dispatch_id, int agv_id) {return true;} //强制结束任务
    virtual bool taskFinishedNotify(std::string dispatch_id, int agv_id){return true;} //任务结束, 用在取空卡赛结束
    virtual AgvTaskPtr getTaskByDisPatchID(std::string dispatch_id) {return nullptr;}
    //群创接口 end

protected:
    TaskMaker();
};

#endif // TASKMAKER_H
