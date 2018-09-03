#include "taskmanager.h"
#include "common.h"
#include "mapmap/mapmanager.h"
#include "agvmanager.h"
#include "sqlite3/CppSQLite3.h"
#include "userlogmanager.h"
#include "taskmaker.h"
#include "Dongyao/dyforklift.h"
#ifdef QUNCHUANG_PROJECT
#include "agvImpl/ros/agv/rosAgv.h"
#endif

TaskManager::TaskManager() : node_id(0),
    thing_id(0),
    task_id(0)
{
}

void TaskManager::checkTable()
{
    try
    {
        if (!g_db.tableExists("agv_task"))
        {
            g_db.execDML("create table agv_task(id INTEGER,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code INTEGER,agv INTEGER,status INTEGER,priority INTEGER,dongindex INTEGER) ;");
        }
        if (!g_db.tableExists("agv_task_node"))
        {
            g_db.execDML("create table agv_task_node(id INTEGER,taskId INTEGER,aimstation INTEGER);");
        }
        if (!g_db.tableExists("agv_task_node_thing"))
        {
            g_db.execDML("create table agv_task_node_thing(id INTEGER,task_node_id INTEGER,discribe char[256]);");
        }
    }
    catch (CppSQLite3Exception &e)
    {
        combined_logger->error("sqlite error {0}:{1}", e.errorCode(), e.errorMessage());
        return;
    }
    catch (std::exception e)
    {
        combined_logger->error("sqlite error {0}", e.what());
        return;
    }
}

bool TaskManager::hasTaskDoing()
{
    return !toDistributeTasks.empty() && !doingTask.empty();
}

bool TaskManager::distributeTask(AgvTaskPtr task)
{
    auto mapmanagerptr = MapManager::getInstance();

    std::vector<AgvTaskNodePtr> nodes = task->getTaskNodes();
    int index = task->getDoingIndex();
    if (index >= nodes.size())
    {
        //任务完成了
        int runTimes = task->getRunTimes() - 1;

        combined_logger->info("{0} 任务完成了, 还需{1}个循环 ", task->getId(), runTimes);

        if (runTimes <= 0)
        {
//            AgvPtr agv = AgvManager::getInstance()->getAgvById(task->getAgv());
//            agv->status = Agv::AGV_STATUS_IDLE;
            finishTask(task);
            return true;
        }
        else
        {
            task->setDoingIndex(0);
            task->setRunTimes(runTimes);
        }
    }
    else
    {
        AgvTaskNodePtr node = nodes[index];
        int aimStation = node->getStation();
        AgvPtr agv = AgvManager::getInstance()->getAgvById(task->getAgv());
        if (agv != nullptr && aimStation == 0 && agv->getTask() == task)
        {
            //拿去执行//从未分配队列中拿出去agv
            excuteTask(task);
            return true;
        }
        else
        {
            if (agv == nullptr)
            {
                //未分配AGV
                AgvPtr bestAgv = nullptr;
                int minDis = DISTANCE_INFINITY;
                std::vector<int> result;
                //遍历所有的agv
                AgvManager::getInstance()->foreachAgv(
                            [&](AgvPtr tempagv) {
                    if (tempagv->status != Agv::AGV_STATUS_IDLE)
                    {
                        //combined_logger->error(" tempagv->status != Agv::AGV_STATUS_IDLE return... ");
                        return;
                    }
                    if (tempagv->getNowStation() != 0)
                    {
                        int tempDis;

                        std::vector<int> result_temp;

                        result_temp = mapmanagerptr->getBestPath(tempagv->getId(), tempagv->getLastStation(), tempagv->getNowStation(), aimStation, tempDis, CAN_CHANGE_DIRECTION);

                        if (result_temp.size() > 0 && tempDis < minDis)
                        {
                            minDis = tempDis;
                            bestAgv = tempagv;
                            result = result_temp;
                        }
                    }
                    else
                    {
                        int tempDis;

                        std::vector<int> result_temp;

                        result_temp = mapmanagerptr->getBestPath(tempagv->getId(), tempagv->getLastStation(), tempagv->getNextStation(), aimStation, tempDis, CAN_CHANGE_DIRECTION);

                        if (result_temp.size() > 0 && tempDis < minDis)
                        {
                            minDis = tempDis;
                            bestAgv = tempagv;
                            result = result_temp;
                        }
                    }
                });

                if (bestAgv != NULL && minDis != DISTANCE_INFINITY)
                {
                    //找到了最优线路和最佳agv
                    combined_logger->info(" 找到了最优线路和最佳agv {0}", bestAgv->getId());
                    mapmanagerptr->addOccuStation(aimStation, bestAgv);
                    for (auto tline : result)
                    {
                        mapmanagerptr->addOccuLine(tline, bestAgv);
                    }
                    bestAgv->setTask(task);
                    task->setPath(result);
                    task->setAgv(bestAgv->getId());
                    bestAgv->onTaskStart(task);
                    excuteTask(task);
                    return true;
                }
                else
                {
                    //未能找到车辆和线路
                    bestAgv = nullptr;
                    minDis = DISTANCE_INFINITY;
                    result.clear();
                    int haltStation = -1;


                    //寻找空闲车辆到该避让点等待
                    AgvManager::getInstance()->foreachAgv(
                                [&](AgvPtr tempagv) {
                        if (tempagv->status != Agv::AGV_STATUS_IDLE)
                        {
                            return;
                        }

                        //1.寻找距离目的地最近的避让点
                        int haltStationTemp = mapmanagerptr->getNearestHaltStation(tempagv->getId(),aimStation);

                        //2.寻找该车到达 躲避点的线路
                        if (tempagv->getNowStation() != 0)
                        {
                            int tempDis;

                            std::vector<int> result_temp;

                            result_temp = mapmanagerptr->getBestPath(tempagv->getId(), tempagv->getLastStation(), tempagv->getNowStation(), haltStationTemp, tempDis, CAN_CHANGE_DIRECTION);

                            if (result_temp.size() > 0 && tempDis < minDis)
                            {
                                minDis = tempDis;
                                bestAgv = tempagv;
                                result = result_temp;
                                haltStation = haltStationTemp;
                            }
                        }
                        else
                        {
                            int tempDis;

                            std::vector<int> result_temp;

                            result_temp = mapmanagerptr->getBestPath(tempagv->getId(), tempagv->getLastStation(), tempagv->getNextStation(), haltStation, tempDis, CAN_CHANGE_DIRECTION);

                            if (result_temp.size() > 0 && tempDis < minDis)
                            {
                                minDis = tempDis;
                                bestAgv = tempagv;
                                result = result_temp;
                                haltStation = haltStationTemp;
                            }
                        }
                    });

                    //找到了空闲车辆去往避让点
                    if (bestAgv != nullptr && minDis != DISTANCE_INFINITY && haltStation != -1)
                    {
                        combined_logger->info(" 找到了去往躲避点{0} 的最优线路和最佳agv {1}", haltStation, bestAgv->getId());

                        if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING ||GLOBAL_AGV_PROJECT == AGV_PROJECT_QINGDAO){
                            mapmanagerptr->addOccuStation(haltStation, bestAgv);
                            for (auto tline : result)
                            {
                                mapmanagerptr->addOccuLine(tline, bestAgv);
                            }


                            //在前面增加一个 去往躲避点的节点，
                            AgvTaskNodePtr node_node(new AgvTaskNode());
                            node_node->setStation(haltStation);
                            node_node->setTaskType(TASK_MOVE);
                            task->push_frontNode(node_node);

                            bestAgv->setTask(task);
                            task->setPath(result);
                            task->setAgv(bestAgv->getId());
                            bestAgv->onTaskStart(task);

                            excuteTask(task);
                            return true;
                        }
                    }
                }
            }
            else
            {
                //已分配AGV
                //指定车辆不空闲
                if (agv->getTask() != task)
                {
                    if (agv->status != Agv::AGV_STATUS_IDLE)
                    {
                        //combined_logger->info(" 指定车辆不空闲 ");
                        return false;
                    }
                }
                int distance = DISTANCE_INFINITY;
                //combined_logger->info("before best path {0} {1} {2}", agv->getLastStation(), agv->getNowStation(), aimStation);

                std::vector<int> result;

                result = mapmanagerptr->getBestPath(agv->getId(), agv->getLastStation(), agv->getNowStation(), aimStation, distance, CAN_CHANGE_DIRECTION);

                if (distance != DISTANCE_INFINITY && result.size() > 0)
                {
                    //拿去执行//从未分配队列中拿出去
                    combined_logger->info(" 从未分配队列中拿出去 ");
                    agv->setTask(task);
                    task->setPath(result);
                    combined_logger->info("path={0}", task->getPath().at(0));
                    combined_logger->info("3.excuteTask={0}", task->getId());
                    //占用线路和站点
                    mapmanagerptr->addOccuStation(aimStation, agv);
                    for (auto tline : result)
                    {
                        mapmanagerptr->addOccuLine(tline, agv);
                    }
                    excuteTask(task);
                    return true;
                }
                else
                {
                    //如果agv已经在躲避点了
                    auto nowSstaion = static_cast<MapPoint *>(mapmanagerptr->getMapSpiritById(agv->getNowStation()));
                    if (nowSstaion != nullptr && nowSstaion->getPointType() == MapPoint::Map_Point_Type_HALT) {
                        return false;
                    }

                    distance = DISTANCE_INFINITY;
                    result.clear();
                    //没有合适的路程
                    //如果有空闲车辆，那么先执行到 距离目的地最近的避让点
                    //1.寻找距离目的地最近的避让点
                    int haltStation = mapmanagerptr->getNearestHaltStation(agv->getId(),aimStation);
                    if(haltStation == -1) return false;
                    //2.寻找车辆到该避让点等待

                    result = mapmanagerptr->getBestPath(agv->getId(), agv->getLastStation(), agv->getNowStation(), haltStation, distance, CAN_CHANGE_DIRECTION);

                    //找到了空闲车辆去往避让点
                    if (distance != DISTANCE_INFINITY && result.size() > 0)
                    {
                        combined_logger->info(" 找到了去往避让点{0} 的最优线路 agv {1}", haltStation, agv->getId());
                        if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING ||GLOBAL_AGV_PROJECT == AGV_PROJECT_QINGDAO ){
                            mapmanagerptr->addOccuStation(haltStation, agv);
                            for (auto tline : result)
                            {
                                mapmanagerptr->addOccuLine(tline, agv);
                            }

                            //在前面增加一个 去往躲避点的节点，
                            AgvTaskNodePtr node_node(new AgvTaskNode());
                            node_node->setStation(haltStation);
                            node_node->setTaskType(TASK_MOVE);
                            task->push_frontNode(node_node);


                            agv->setTask(task);
                            task->setPath(result);
                            task->setAgv(agv->getId());
                            agv->onTaskStart(task);

                            excuteTask(task);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool TaskManager::init()
{
    //check table
    checkTable();
    task_id = 1;
    node_id = 1;
    thing_id = 1;
    try
    {
        //找到最小的ID
        task_id = g_db.execScalar("select max(id) from agv_task;");
        node_id = g_db.execScalar("select max(id) from agv_task_node;");
        thing_id = g_db.execScalar("select max(id) from agv_task_node_thing;");
    }
    catch (CppSQLite3Exception &e)
    {
        combined_logger->error("sqlite error {0}:{1}", e.errorCode(), e.errorMessage());
    }
    catch (std::exception e)
    {
        combined_logger->error("sqlite error {0}", e.what());
    }

    //启动一个分配任务的线程
    g_threadPool.enqueue([&] {
        while (!g_quit)
        {
            toDisMtx.lock();
            for (auto itr = toDistributeTasks.begin(); itr != toDistributeTasks.end() && !g_quit; ++itr)
            {
                for (auto pos = itr->second.begin(); pos != itr->second.end();)
                {
                    AgvTaskPtr task = *pos;
                    //分配任务
                    bool d = distributeTask(task);
                    if(d){
                        //任务 分配 OK
                        pos = itr->second.erase(pos);
                    }else{
                        //任务 分配 fail
                        ++pos;
                    }
                }
            }
            toDisMtx.unlock();
            usleep(20000);
        }
    });

    return true;
}

//添加任务
bool TaskManager::addTask(AgvTaskPtr task)
{
    if (task->getId() <= 0)
        task->setId(++task_id);
    bool add = false;
    toDisMtx.lock();

    for (auto itr = toDistributeTasks.begin(); itr != toDistributeTasks.end(); ++itr)
    {
        if (itr->first == task->getPriority())
        {
            itr->second.push_back(task);
            add = true;
            break;
        }
    }
    if (!add)
    {
        std::vector<AgvTaskPtr> vs;
        vs.push_back(task);
        toDistributeTasks.insert(std::make_pair(task->getPriority(), vs));
    }
    toDisMtx.unlock();
    return true;
}

//查询未执行的任务
AgvTaskPtr TaskManager::queryUndoTask(int taskId)
{

    return nullptr;
}

//查询正在执行的任务
AgvTaskPtr TaskManager::queryDoingTask(int taskId)
{

    return nullptr;
}

//查询完成执行的任务
AgvTaskPtr TaskManager::queryDoneTask(int taskId)
{

    return nullptr;
}

//返回task的状态。
int TaskManager::queryTaskStatus(int taskId)
{
    AgvTaskPtr task = queryUndoTask(taskId);
    if (task != nullptr)
    {
        return AgvTask::AGV_TASK_STATUS_UNEXCUTE;
    }

    task = queryDoingTask(taskId);
    if (task != nullptr)
    {
        return AgvTask::AGV_TASK_STATUS_EXCUTING;
    }

    task = queryDoneTask(taskId);
    if (task != nullptr)
    {
        return task->getStatus();
    }
    return AgvTask::AGV_TASK_STATUS_UNEXIST;
}

//保存到数据库
bool TaskManager::saveTask(AgvTaskPtr task)
{
    try
    {
        if (!g_db.tableExists("agv_task"))
        {
            g_db.execDML("create table agv_task(id INTEGER,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code INTEGER,agv INTEGER,status INTEGER,priority INTEGER,dongindex INTEGER) ;");
        }
        if (!g_db.tableExists("agv_task_node"))
        {
            g_db.execDML("create table agv_task_node(id INTEGER,taskId INTEGER,aimstation INTEGER);");
        }
        if (!g_db.tableExists("agv_task_node_thing"))
        {
            g_db.execDML("create table agv_task_node_thing(id INTEGER,task_node_id INTEGER,discribe char[256]);");
        }

        g_db.execDML("begin transaction;");
        char buf[SQL_MAX_LENGTH];
        snprintf(buf, SQL_MAX_LENGTH, "insert into agv_task values (%d,'%s','%s','%s','%s','%s','%s',%d,%d,%d,%d,%d);", task->getId(), task->getProduceTime().c_str(), task->getDoTime().c_str(), task->getDoneTime().c_str(), task->getCancelTime().c_str(), task->getErrorTime().c_str(), task->getErrorInfo().c_str(), task->getErrorCode(), task->getAgv(), task->getStatus(), task->getPriority(), task->getDoingIndex());
        g_db.execDML(buf);

        for (auto node : task->getTaskNodes())
        {
            int node__id = ++node_id;
            snprintf(buf, SQL_MAX_LENGTH, "insert into agv_task_node values (%d, %d,%d);", node__id, task->getId(), node->getStation());
            g_db.execDML(buf);
            for (auto thing : node->getDoThings())
            {
                int thing__id = ++thing_id;
                snprintf(buf, SQL_MAX_LENGTH, "insert into agv_task_node_thing values (%d, %d,'%s');", thing__id, node__id, thing->discribe().c_str());
                g_db.execDML(buf);
            }
        }
        g_db.execDML("commit transaction;");
    }
    catch (CppSQLite3Exception &e)
    {
        combined_logger->error("sqlite error {0}:{1}", e.errorCode(), e.errorMessage());
        return false;
    }
    catch (std::exception e)
    {
        combined_logger->error("sqlite error {0}", e.what());
        return false;
    }
    return true;
}

//取消一个任务
int TaskManager::cancelTask(int taskId)
{
    bool alreadyCancel = false;
    AgvTaskPtr task = nullptr;
    toDisMtx.lock();
    for (auto itr = toDistributeTasks.begin(); itr != toDistributeTasks.end(); ++itr)
    {
        for (auto pos = itr->second.begin(); pos != itr->second.end();)
        {
            if ((*pos)->getId() == taskId)
            {
                task = *pos;
                pos = itr->second.erase(pos);
                alreadyCancel = true;
            }
            else
            {
                ++pos;
            }
        }
    }
    toDisMtx.unlock();

    if (!alreadyCancel)
    {
        //查找正在执行的任务
        doingTaskMtx.lock();
        for (auto t : doingTask)
        {
            if (t->getId() == taskId)
            {
                task = t;
                //TODO 释放占有的路径等
                doingTask.erase(std::find(doingTask.begin(), doingTask.end(), task));
                alreadyCancel = true;
                break;
            }
        }
        doingTaskMtx.unlock();
    }

    if (task != nullptr)
    {
        task->cancel();
        task->setCancelTime(getTimeStrNow());
        //保存到数据库
        saveTask(task);
        //设置状态
        AgvPtr agv = AgvManager::getInstance()->getAgvById(task->getAgv());
        if (agv != nullptr)
        {
            agv->setTask(nullptr);
            agv->cancelTask();
            //TODO:
            agv->status = Agv::AGV_STATUS_IDLE;
        }
        task->setStatus(AgvTask::AGV_TASK_STATUS_CANCEL);
    }

    return 0;
}

//完成了一个任务
void TaskManager::finishTask(AgvTaskPtr task)
{
    //设置完成时间
    task->setDoneTime(getTimeStrNow());

    //保存到数据库
    saveTask(task);

    //设置状态
    AgvPtr agv = AgvManager::getInstance()->getAgvById(task->getAgv());
    if (agv != nullptr)
    {
        agv->onTaskFinished(task);
        agv->setTask(nullptr);
        //TODO:
        agv->status = Agv::AGV_STATUS_IDLE;
    }
    //
    doneTaskMtx.lock();
    doneTask.push_back(task);
    doneTaskMtx.unlock();
    task->setStatus(AgvTask::AGV_TASK_STATUS_DONE);
}

void TaskManager::excuteTask(AgvTaskPtr task)
{
    //启动一个线程,去执行该任务
    doingTaskMtx.lock();
    doingTask.push_back(task);
    doingTaskMtx.unlock();
    g_threadPool.enqueue([&, task] {
        task->setStatus(AgvTask::AGV_TASK_STATUS_EXCUTING);
        std::vector<AgvTaskNodePtr> nodes = task->getTaskNodes();
        int index = task->getDoingIndex();
        if(index == 0)task->setDoTime(getTimeStrNow());
        if (index >= nodes.size())
        {
            //任务完成了
            doingTaskMtx.lock();
            doingTask.erase(std::find(doingTask.begin(), doingTask.end(), task));
            doingTaskMtx.unlock();
            addTask(task);
            combined_logger->info("task->finish");
        }
        else
        {
            AgvTaskNodePtr node = nodes[index];
            int station = node->getStation();

            auto agv = AgvManager::getInstance()->getAgvById(task->getAgv());

            try
            {
                if (station == NULL)
                {
                    for (auto thing : node->getDoThings())
                    {
                        combined_logger->info("station==null for dothings");
                        if (task->getIsCancel())
                            break;
                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);
                        if (thing->result(agv))
                            break;
                    }
                }
                else
                {
                    //去往这个点位
                    combined_logger->info("task->pathsize={0}", task->getPath().size());
                    combined_logger->info("agv->id={0}", agv->getId());
                    agv->excutePath(task->getPath());

                    //执行任务
                    for (auto thing : node->getDoThings())
                    {
                        combined_logger->info("for dothings nodesize={0}", node->getDoThings().size());
                        if (task->getIsCancel())
                            break;
                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);
                    }
                }
                //完成以后,从正在执行，返回到 分配队列中
                task->setPath(std::vector<int>()); //清空路径
                task->setDoingIndex(task->getDoingIndex() + 1);
                if (!task->getIsCancel())
                {
                    doingTaskMtx.lock();
                    doingTask.erase(std::find(doingTask.begin(), doingTask.end(), task));
                    doingTaskMtx.unlock();
                    addTask(task);
                    combined_logger->info("addtask={0}-{1}", task->getId(), task->getDoingIndex());
                }
            }
            catch (std::exception e)
            {
                //TODO:执行过程中发生异常
                //TODO:
            }
        }
    });
}

std::vector<AgvTaskPtr> TaskManager::getCurrentTasks()
{
    std::vector<AgvTaskPtr> tasks;
    toDisMtx.lock();
    for (auto d : toDistributeTasks)
    {
        auto tTs = d.second;
        for (auto t : tTs)
        {
            tasks.push_back(t);
        }
    }
    toDisMtx.unlock();

    doingTaskMtx.lock();
    for (auto d : doingTask)
    {
        tasks.push_back(d);
    }
    doingTaskMtx.unlock();

    doneTaskMtx.lock();
    for (auto d : doneTask)
    {
        tasks.push_back(d);
    }
    doneTaskMtx.unlock();

    return tasks;
}

void TaskManager::interCreate(SessionPtr conn, const Json::Value &request)
{
    TaskMaker::getInstance()->makeTask(conn, request);
}

void TaskManager::interQueryStatus(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    if (request["id"].isNull())
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else
    {
        int id = request["id"].asInt();
        UserLogManager::getInstance()->push(conn->getUserName() + " query task status,with task ID:" + intToString(id));
        int status = queryTaskStatus(id);
        response["status"] = status;
    }
    conn->send(response);
}

void TaskManager::interCancel(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    if (request["id"].isNull())
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else
    {
        int id = request["id"].asInt();
        UserLogManager::getInstance()->push(conn->getUserName() + " cancel task，task ID:" + intToString(id));
        //取消该任务
        cancelTask(id);
    }
    conn->send(response);
}

void TaskManager::interListUndo(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    UserLogManager::getInstance()->push(conn->getUserName() + " query undo tasks list");
    Json::Value task_infos;
    toDisMtx.lock();
    for (auto mmaptask : toDistributeTasks)
    {
        for (auto task : mmaptask.second)
        {
            Json::Value info;
            info["id"] = task->getId();
            info["produceTime"] = task->getProduceTime();
            info["excuteAgv"] = task->getAgv();
            info["status"] = AgvTask::AGV_TASK_STATUS_EXCUTING;
            task_infos.append(info);
        }
    }
    toDisMtx.unlock();
    response["tasks"] = task_infos;

    conn->send(response);
}

void TaskManager::interListDoing(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    UserLogManager::getInstance()->push(conn->getUserName() + " query doing task list");
    Json::Value task_infos;
    doingTaskMtx.lock();
    for (auto task : doingTask)
    {
        Json::Value info;

        info["id"] = task->getId();
        info["produceTime"] = task->getProduceTime();
        info["doTime"] = task->getDoTime();
        info["excuteAgv"] = task->getAgv();
        info["status"] = AgvTask::AGV_TASK_STATUS_EXCUTING;
        task_infos.append(info);
    }
    doingTaskMtx.unlock();
    response["tasks"] = task_infos;

    conn->send(response);
}

void TaskManager::interListDoneToday(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    std::stringstream ss;
    ss << "select id,produce_time,do_time,done_time,agv from agv_task where done_time<= \'" << getTimeStrTomorrow() << "\' and done_time>= \'" << getTimeStrToday() << "\';";
    UserLogManager::getInstance()->push(conn->getUserName() + " query task done today list");
    try
    {
        CppSQLite3Table table = g_db.getTable(ss.str().c_str());
        Json::Value task_infos;
        for (int i = 0; i < table.numRows(); ++i)
        {
            table.setRow(i);
            Json::Value info;

            info["id"] = std::atoi(table.fieldValue(0));
            info["produceTime"] = std::string(table.fieldValue(1));
            info["doTime"] = std::string(table.fieldValue(2));
            info["doneTime"] = std::string(table.fieldValue(3));
            info["excuteAgv"] = std::atoi(table.fieldValue(4));
            info["status"] = AgvTask::AGV_TASK_STATUS_DONE;

            task_infos.append(info);
        }
        response["tasks"] = task_infos;
    }
    catch (CppSQLite3Exception e)
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        std::stringstream ss;
        ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
        response["error_info"] = ss.str();
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
    }
    catch (std::exception e)
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        std::stringstream ss;
        ss << "info:" << e.what();
        response["error_info"] = ss.str();
        combined_logger->error("sqlerr code:{0}", e.what());
    }

    conn->send(response);
}

void TaskManager::interListDuring(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    if (request["startTime"].isNull() ||
            request["endTime"].isNull())
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else
    {
        std::string startTime = request["startTime"].asString();
        std::string endTime = request["endTime"].asString();
        UserLogManager::getInstance()->push(conn->getUserName() + " query history tasks list ，time from " + startTime + " to" + endTime);
        std::stringstream ss;
        ss << "select id,produce_time,do_time,done_time,agv from agv_task where done_time<= \'" << endTime << "\' and done_time>= \'" << startTime << "\';";

        try
        {
            CppSQLite3Table table = g_db.getTable(ss.str().c_str());
            Json::Value task_infos;
            for (int i = 0; i < table.numRows(); ++i)
            {
                table.setRow(i);

                Json::Value info;
                info["id"] = std::atoi(table.fieldValue(0));
                info["produceTime"] = std::string(table.fieldValue(1));
                info["doTime"] = std::string(table.fieldValue(2));
                info["doneTime"] = std::string(table.fieldValue(3));
                info["excuteAgv"] = std::atoi(table.fieldValue(4));
                info["status"] = AgvTask::AGV_TASK_STATUS_DONE;
                task_infos.append(info);
            }
            response["tasks"] = task_infos;
        }
        catch (CppSQLite3Exception e)
        {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
            response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        }
        catch (std::exception e)
        {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "info:" << e.what();
            response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0}", e.what());
        }
    }
    conn->send(response);
}
