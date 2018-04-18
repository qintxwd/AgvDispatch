#include "taskmanager.h"
#include "Common.h"
#include "mapmanager.h"
#include "agvmanager.h"
#include "sqlite3/CppSQLite3.h"

TaskManager *TaskManager::p = new TaskManager();

//最多20个任务同时执行.20个线程的线程池

TaskManager::TaskManager():
    pool(20),
    node_id(0),
    thing_id(0),
    task_id(0)
{
}

void TaskManager::checkTable()
{
    try{
        CppSQLite3DB db;
        db.open(DB_File);
        if(!db.tableExists("agv_task")){
            db.execDML("create table agv_task(id int,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code int,agv int,status int,priority int,dongindex int) ;");
        }
        if(!db.tableExists("agv_task_node")){
            db.execDML("create table agv_task_node(id int,taskId int,aimstation int);");
        }
        if(!db.tableExists("agv_task_node_thing")){
            db.execDML("create table agv_task_node_thing(id int,task_node_id int,discribe char[256]);");
        }
    }catch(CppSQLite3Exception &e){
        std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
        return ;
    }catch(std::exception e){
        std::cerr << e.what()  << std::endl;
        return ;
    }
}

bool TaskManager::init()
{
    //check table
    checkTable();
    task_id = 1;
    node_id = 1;
    thing_id = 1;
    try{
        CppSQLite3DB db;
        db.open(DB_File);
        //找到最小的ID
        task_id = db.execScalar("select max(id) from agv_task;");
        node_id = db.execScalar("select max(id) from agv_task_node;");
        thing_id = db.execScalar("select max(id) from agv_task_node_thing;");
    }catch(CppSQLite3Exception &e){
        std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
    }catch(std::exception e){
        std::cerr << e.what()  << std::endl;
    }

    //启动一个分配任务的线程
    pool.enqueue([&]{
        while(true){
            toDisMtx.lock();
            for(auto itr = toDistributeTasks.begin();itr!=toDistributeTasks.end();++itr){
                for(auto pos = itr->second.begin();pos!=itr->second.end();){
                    AgvTask *task = *pos;
                    std::vector<AgvTaskNode *> nodes = task->getTaskNode();
                    int index = task->getDoingIndex();
                    if(index >= nodes.size()){
                        //任务完成了
                        pos = itr->second.erase(pos);
                        finishTask(task);
                        continue;
                    }else{
                        AgvTaskNode *node = nodes[index];
                        AgvStation *aimStation = node->getStation();
                        if(aimStation==NULL && task->getAgv()->getTask() == task){
                            //拿去执行//从未分配队列中拿出去agv
                            pos = itr->second.erase(pos);
                            excuteTask(task);
                            continue;
                        }else{
                            //获取执行路径
                            Agv *agv = task->getAgv();
                            if(agv == NULL){
                                //未分配AGV
                                Agv *bestAgv = NULL;
                                int minDis = DISTANCE_INFINITY;
                                std::vector<AgvLine *> result;
                                //遍历所有的agv
                                AgvManager::getInstance()->foreachAgv(
                                            [&](Agv *tempagv){
                                    if(tempagv->status!=Agv::AGV_STATUS_IDLE)
                                        return ;
                                    if(tempagv->nowStation!=nullptr){
                                        int tempDis;
                                        std::vector<AgvLine *> result_temp = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nowStation,aimStation,tempDis,CAN_CHANGE_DIRECTION);
                                        if(result_temp.size()>0 && tempDis<minDis){
                                            minDis = tempDis;
                                            bestAgv = agv;
                                            result = result_temp;
                                        }
                                    }else{
                                        int tempDis;
                                        std::vector<AgvLine *> result_temp = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nextStation,aimStation,tempDis,CAN_CHANGE_DIRECTION);
                                        if(result_temp.size()>0 && tempDis<minDis){
                                            minDis = tempDis;
                                            bestAgv = agv;
                                            result = result_temp;
                                        }
                                    }
                                });

                                if(bestAgv!=NULL && minDis!=DISTANCE_INFINITY){
                                    //找到了最优线路和最佳agv
                                    bestAgv->setTask(task);
                                    task->setPath(result);
                                    pos = itr->second.erase(pos);
                                    excuteTask(task);
                                    continue;
                                }
                            }else{
                                //已分配AGV
                                //指定车辆不空闲
                                if(agv->getTask()!=task){
                                    if(agv->status != Agv::AGV_STATUS_IDLE)
                                        continue;
                                }
                                int distance;
                                std::vector<AgvLine *> result = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nowStation,aimStation,distance,CAN_CHANGE_DIRECTION);

                                if(distance!=DISTANCE_INFINITY && result.size()>0){
                                    //拿去执行//从未分配队列中拿出去
                                    agv->setTask(task);
                                    task->setPath(result);
                                    pos = itr->second.erase(pos);
                                    excuteTask(task);
                                    continue;
                                }
                            }
                        }
                    }
                    ++pos;
                }
            }
            toDisMtx.unlock();
            std::this_thread::sleep_for(duration_millisecond(500));
        }
    });

    return true;
}

//添加任务
bool TaskManager::addTask(AgvTask *task)
{
    task->setId(++task_id);
    bool add = false;
    toDisMtx.lock();
    for(auto itr = toDistributeTasks.begin();itr!=toDistributeTasks.end();++itr){
        if(itr->first == task->getPriority()){
            itr->second.push_back(task);
            add = true;
            break;
        }
    }
    if(!add){
        std::vector<AgvTask *> vs;
        vs.push_back(task);
        toDistributeTasks.insert(std::make_pair(task->getPriority(),vs));
    }
    toDisMtx.unlock();
    return true;
}

//查询未执行的任务
AgvTask *TaskManager::queryUndoTask(int taskId)
{

    return nullptr;
}

//查询正在执行的任务
AgvTask *TaskManager::queryDoingTask(int taskId)
{

    return nullptr;
}

//查询完成执行的任务
AgvTask *TaskManager::queryDoneTask(int taskId)
{

    return nullptr;
}

//返回task的状态。
int TaskManager::queryTaskStatus(int taskId)
{

    return 0;
}

//保存到数据库
bool TaskManager::saveTask(AgvTask *task)
{
    try{
        CppSQLite3DB db;
        db.open(DB_File);
        if(!db.tableExists("agv_task")){
            db.execDML("create table agv_task(id int,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code int,agv int,status int,priority int,dongindex int) ;");
        }
        if(!db.tableExists("agv_task_node")){
            db.execDML("create table agv_task_node(id int,taskId int,aimstation int);");
        }
        if(!db.tableExists("agv_task_node_thing")){
            db.execDML("create table agv_task_node_thing(id int,task_node_id int,discribe char[256]);");
        }

        db.execDML("begin transaction;");
        char buf[1024];
        sprintf(buf, "insert into agv_task values (%d,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d);", task->getId(),task->getProduceTime().c_str(),task->getDoTime().c_str(),task->getDoneTime().c_str(),task->getCancelTime().c_str(),task->getErrorTime().c_str(),task->getErrorInfo().c_str(),task->getErrorCode(),task->getAgv()->getId(),task->getStatus(),task->getPriority(),task->getDoingIndex());
        db.execDML(buf);

        for(auto node:task->getTaskNode()){
            int node__id = ++node_id;
            sprintf(buf, "insert into agv_task_node values (%d, %d,%d);", node__id, task->getId(),node->getStation()->id);
            db.execDML(buf);
            for(auto thing:node->getDoThings()){
                int thing__id = ++thing_id;
                sprintf(buf, "insert into agv_task_node_thing values (%d, %d,%s);", thing__id, node__id,thing->discribe().c_str());
                db.execDML(buf);
            }
        }
        db.execDML("commit transaction;");
    }catch(CppSQLite3Exception &e){
        std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
        return false;
    }catch(std::exception e){
        std::cerr << e.what()  << std::endl;
        return false;
    }
    return true;
}

//取消一个任务
int TaskManager::cancelTask(int taskId)
{
    bool alreadyCancel = false;
    AgvTask *task = nullptr;
    toDisMtx.lock();
    for(auto itr = toDistributeTasks.begin();itr!=toDistributeTasks.end();++itr){
        for(auto pos = itr->second.begin();pos!=itr->second.end();){
            if((*pos)->getId() == taskId){
                task = *pos;
                pos = itr->second.erase(pos);
                alreadyCancel = true;
            }else{
                ++pos;
            }
        }
    }
    toDisMtx.unlock();

    if(!alreadyCancel){
        //查找正在执行的任务
        doingTaskMtx.lock();
        for(auto t:doingTask){
            if(t->getId() == taskId){
                task = t;
                doingTask.erase(std::find(doingTask.begin(),doingTask.end(),task));
                alreadyCancel = true;
                break;
            }
        }
        doingTaskMtx.unlock();
    }


    if(task!=nullptr){
        task->setCancelTime(getTimeStrNow());
        //保存到数据库
        saveTask(task);
        //设置状态
        task->getAgv()->setTask(nullptr);
        task->getAgv()->status = Agv::AGV_STATUS_IDLE;

        //删除任务
        for(auto n:task->getTaskNode()){
            for(auto t:n->getDoThings()){
                delete t;
            }
            delete n;
        }
        delete task;
    }

    return 0;
}

//完成了一个任务
void TaskManager::finishTask(AgvTask *task)
{
    //设置完成时间
    task->setDoneTime(getTimeStrNow());

    //保存到数据库
    saveTask(task);

    //设置状态
    task->getAgv()->setTask(nullptr);
    task->getAgv()->status = Agv::AGV_STATUS_IDLE;

    //删除任务
    for(auto n:task->getTaskNode()){
        for(auto t:n->getDoThings()){
            delete t;
        }
        delete n;
    }
    delete task;
}

void TaskManager::excuteTask(AgvTask *task)
{
    //启动一个线程,去执行该任务
    doingTaskMtx.lock();
    doingTask.push_back(task);
    doingTaskMtx.unlock();
    pool.enqueue([&]{
        std::vector<AgvTaskNode *> nodes = task->getTaskNode();
        int index = task->getDoingIndex();
        if(index >= nodes.size()){
            //任务完成了
            doingTaskMtx.lock();
            doingTask.erase(std::find(doingTask.begin(),doingTask.end(),task));
            doingTaskMtx.unlock();
            finishTask(task);
        }else{
            AgvTaskNode *node = nodes[index];
            AgvStation *station = node->getStation();
            Agv *agv = task->getAgv();
            try{
                if(station==NULL){
                    for(auto thing:node->getDoThings()){

                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);

                    }
                }else{
                    //去往这个点位
                    agv->excutePath(task->getPath());
                    //执行任务
                    for(auto thing:node->getDoThings()){
                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);
                    }
                }
                //完成以后,从正在执行，返回到 分配队列中
                task->setPath(std::vector<AgvLine *>());//清空路径
                task->setDoingIndex(task->getDoingIndex()+1);
                doingTask.erase(std::find(doingTask.begin(),doingTask.end(),task));
                addTask(task);
            }catch(std::exception e){
                //TODO:执行过程中发生异常
                //TODO:
            }
        }
    });
}
