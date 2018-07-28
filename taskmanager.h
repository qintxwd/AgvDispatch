#ifndef TASKMANAGER_H
#define TASKMANAGER_H
#include <mutex>
#include <map>
#include <atomic>

#include "agvtask.h"
#include "utils/noncopyable.h"

#include "network/session.h"

class TaskManager;

using TaskManagerPtr = std::shared_ptr<TaskManager>;

class TaskManager :  public noncopyable,public std::enable_shared_from_this<TaskManager>
{
public:
    static TaskManagerPtr getInstance(){
        static TaskManagerPtr m_inst = TaskManagerPtr(new TaskManager());
        return m_inst;
    }
    bool init();

    bool hasTaskDoing();

    //添加任务
    bool addTask(AgvTaskPtr task);

    //查询未执行的任务
    AgvTaskPtr queryUndoTask(int taskId);

    //查询正在执行的任务
    AgvTaskPtr queryDoingTask(int taskId);

    //查询完成执行的任务
    AgvTaskPtr queryDoneTask(int taskId);

    //返回task的状态。
    int queryTaskStatus(int taskId);

    //取消一个任务
    int cancelTask(int taskId);

    //完成了一个任务
    void finishTask(AgvTaskPtr task);

    //执行一个任务
    void excuteTask(AgvTaskPtr task);

	std::vector<AgvTaskPtr> getCurrentTasks();

    //用户接口
    void interCreate(SessionPtr conn, const Json::Value &request);
    void interQueryStatus(SessionPtr conn, const Json::Value &request);
    void interCancel(SessionPtr conn, const Json::Value &request);
    void interListUndo(SessionPtr conn, const Json::Value &request);
    void interListDoing(SessionPtr conn, const Json::Value &request);
    void interListDoneToday(SessionPtr conn, const Json::Value &request);
    void interListDuring(SessionPtr conn, const Json::Value &request);
protected:
    TaskManager();
private:
    void checkTable();
    //未分配的任务[优先级/任务列表]
    std::map<int, std::vector<AgvTaskPtr> ,std::greater<int> > toDistributeTasks;
    std::mutex toDisMtx;

    //已分配的任务
    std::vector<AgvTaskPtr> doingTask;
    std::mutex doingTaskMtx;

	//已完成的任务
	std::vector<AgvTaskPtr> doneTask;
	std::mutex doneTaskMtx;

    bool saveTask(AgvTaskPtr task);

    std::atomic_int node_id;
    std::atomic_int thing_id;
    std::atomic_int task_id;
};

#endif // TASKMANAGER_H
