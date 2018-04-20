#include "taskmanager.h"
#include "Common.h"
#include "mapmanager.h"
#include "agvmanager.h"
#include "sqlite3/CppSQLite3.h"
#include "userlogmanager.h"

TaskManager::TaskManager():
    node_id(0),
    thing_id(0),
    task_id(0)
{
}

void TaskManager::checkTable()
{
    try{
        if(!g_db.tableExists("agv_task")){
            g_db.execDML("create table agv_task(id int,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code int,agv int,status int,priority int,dongindex int) ;");
        }
        if(!g_db.tableExists("agv_task_node")){
            g_db.execDML("create table agv_task_node(id int,taskId int,aimstation int);");
        }
        if(!g_db.tableExists("agv_task_node_thing")){
            g_db.execDML("create table agv_task_node_thing(id int,task_node_id int,discribe char[256]);");
        }
    }catch(CppSQLite3Exception &e){
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage() ;
        return ;
    }catch(std::exception e){
        LOG(ERROR) << e.what()  ;
        return ;
    }
}

bool TaskManager::hasTaskDoing()
{
    return !toDistributeTasks.empty() && !doingTask.empty();
}

bool TaskManager::init()
{
    //check table
    checkTable();
    task_id = 1;
    node_id = 1;
    thing_id = 1;
    try{
        //找到最小的ID
        task_id = g_db.execScalar("select max(id) from agv_task;");
        node_id = g_db.execScalar("select max(id) from agv_task_node;");
        thing_id = g_db.execScalar("select max(id) from agv_task_node_thing;");
    }catch(CppSQLite3Exception &e){
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage() ;
    }catch(std::exception e){
        LOG(ERROR) << e.what()  ;
    }

    //启动一个分配任务的线程
    g_threadPool.enqueue([&]{
        while(true){
            toDisMtx.lock();
            for(auto itr = toDistributeTasks.begin();itr!=toDistributeTasks.end();++itr){
                for(auto pos = itr->second.begin();pos!=itr->second.end();){
                    AgvTaskPtr task = *pos;
                    std::vector<AgvTaskNodePtr> nodes = task->getTaskNode();
                    int index = task->getDoingIndex();
                    if(index >= nodes.size()){
                        //任务完成了
                        pos = itr->second.erase(pos);
                        finishTask(task);
                        continue;
                    }else{
                        AgvTaskNodePtr node = nodes[index];
                        AgvStationPtr aimStation = node->getStation();
                        if(aimStation==nullptr && task->getAgv()->getTask() == task){
                            //拿去执行//从未分配队列中拿出去agv
                            pos = itr->second.erase(pos);
                            excuteTask(task);
                            continue;
                        }else{
                            //获取执行路径
                            AgvPtr agv = task->getAgv();
                            if(agv == NULL){
                                //未分配AGV
                                AgvPtr bestAgv = NULL;
                                int minDis = DISTANCE_INFINITY;
                                std::vector<AgvLinePtr > result;
                                //遍历所有的agv
                                AgvManager::getInstance()->foreachAgv(
                                            [&](AgvPtr tempagv){
                                    if(tempagv->status!=Agv::AGV_STATUS_IDLE)
                                        return ;
                                    if(tempagv->nowStation!=nullptr){
                                        int tempDis;
                                        std::vector<AgvLinePtr> result_temp = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nowStation,aimStation,tempDis,CAN_CHANGE_DIRECTION);
                                        if(result_temp.size()>0 && tempDis<minDis){
                                            minDis = tempDis;
                                            bestAgv = agv;
                                            result = result_temp;
                                        }
                                    }else{
                                        int tempDis;
                                        std::vector<AgvLinePtr> result_temp = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nextStation,aimStation,tempDis,CAN_CHANGE_DIRECTION);
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
                                std::vector<AgvLinePtr > result = MapManager::getInstance()->getBestPath(agv,agv->lastStation,agv->nowStation,aimStation,distance,CAN_CHANGE_DIRECTION);

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
            std::this_thread::sleep_for(duration_millisecond(200));
        }
    });

    return true;
}

//添加任务
bool TaskManager::addTask(AgvTaskPtr task)
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
        std::vector<AgvTaskPtr> vs;
        vs.push_back(task);
        toDistributeTasks.insert(std::make_pair(task->getPriority(),vs));
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
    if(task!=nullptr)
    {
        return AgvTask::AGV_TASK_STATUS_UNEXCUTE;
    }

    task = queryDoingTask(taskId);
    if(task!=nullptr)
    {
        return AgvTask::AGV_TASK_STATUS_EXCUTING;
    }

    task = queryDoneTask(taskId);
    if(task!=nullptr)
    {
        return task->getStatus();
    }
    return AgvTask::AGV_TASK_STATUS_UNEXIST;
}

//保存到数据库
bool TaskManager::saveTask(AgvTaskPtr task)
{
    try{
        if(!g_db.tableExists("agv_task")){
            g_db.execDML("create table agv_task(id int,produce_time char[64],do_time char[64],done_time char[64],cancel_time char[64],error_time char[64],error_info char[256],error_code int,agv int,status int,priority int,dongindex int) ;");
        }
        if(!g_db.tableExists("agv_task_node")){
            g_db.execDML("create table agv_task_node(id int,taskId int,aimstation int);");
        }
        if(!g_db.tableExists("agv_task_node_thing")){
            g_db.execDML("create table agv_task_node_thing(id int,task_node_id int,discribe char[256]);");
        }

        g_db.execDML("begin transaction;");
        char buf[1024];
        sprintf(buf, "insert into agv_task values (%d,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d);", task->getId(),task->getProduceTime().c_str(),task->getDoTime().c_str(),task->getDoneTime().c_str(),task->getCancelTime().c_str(),task->getErrorTime().c_str(),task->getErrorInfo().c_str(),task->getErrorCode(),task->getAgv()->getId(),task->getStatus(),task->getPriority(),task->getDoingIndex());
        g_db.execDML(buf);

        for(auto node:task->getTaskNode()){
            int node__id = ++node_id;
            sprintf(buf, "insert into agv_task_node values (%d, %d,%d);", node__id, task->getId(),node->getStation()->id);
            g_db.execDML(buf);
            for(auto thing:node->getDoThings()){
                int thing__id = ++thing_id;
                sprintf(buf, "insert into agv_task_node_thing values (%d, %d,%s);", thing__id, node__id,thing->discribe().c_str());
                g_db.execDML(buf);
            }
        }
        g_db.execDML("commit transaction;");
    }catch(CppSQLite3Exception &e){
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage() ;
        return false;
    }catch(std::exception e){
        LOG(ERROR) << e.what()  ;
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
        task->cancel();
        task->setCancelTime(getTimeStrNow());
        //保存到数据库
        saveTask(task);
        //设置状态
        task->getAgv()->setTask(nullptr);
        task->getAgv()->cancelTask();
        task->getAgv()->status = Agv::AGV_STATUS_IDLE;
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
    task->getAgv()->setTask(nullptr);
    task->getAgv()->status = Agv::AGV_STATUS_IDLE;

    //删除任务
}

void TaskManager::excuteTask(AgvTaskPtr task)
{
    //启动一个线程,去执行该任务
    doingTaskMtx.lock();
    doingTask.push_back(task);
    doingTaskMtx.unlock();
    g_threadPool.enqueue([&]{
        std::vector<AgvTaskNodePtr> nodes = task->getTaskNode();
        int index = task->getDoingIndex();
        if(index >= nodes.size()){
            //任务完成了
            doingTaskMtx.lock();
            doingTask.erase(std::find(doingTask.begin(),doingTask.end(),task));
            doingTaskMtx.unlock();
            finishTask(task);
        }else{
            AgvTaskNodePtr node = nodes[index];
            AgvStationPtr station = node->getStation();
            AgvPtr agv = task->getAgv();
            try{
                if(station==NULL){
                    for(auto thing:node->getDoThings()){
                        if(task->getIsCancel())break;
                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);
                    }
                }else{
                    //去往这个点位
                    agv->excutePath(task->getPath());
                    //执行任务
                    for(auto thing:node->getDoThings()){
                        if(task->getIsCancel())break;
                        thing->beforeDoing(agv);
                        thing->doing(agv);
                        thing->afterDoing(agv);
                    }
                }
                //完成以后,从正在执行，返回到 分配队列中
                task->setPath(std::vector<AgvLinePtr >());//清空路径
                task->setDoingIndex(task->getDoingIndex()+1);
                if(!task->getIsCancel()){
                    doingTask.erase(std::find(doingTask.begin(),doingTask.end(),task));
                    addTask(task);
                }
            }catch(std::exception e){
                //TODO:执行过程中发生异常
                //TODO:
            }
        }
    });
}

void TaskManager::interCreate(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    //TODO:


}

void TaskManager::interQueryStatus(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=sizeof(int32_t)){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        int32_t id;
        memcpy(&id,msg.body,sizeof(int32_t));
        UserLogManager::getInstance()->push(conn->getUserName()+"请求任务状态，任务ID:"+intToString(id));
        int32_t status = queryTaskStatus(id);
        memcpy(response.body,&status,sizeof(int32_t));
        response.head.body_length = sizeof(int32_t);
    }
    conn->send(response);
}

void TaskManager::interCancel(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=sizeof(int32_t)){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        int32_t id;
        memcpy(&id,msg.body,sizeof(int32_t));
        UserLogManager::getInstance()->push(conn->getUserName()+"取消任务，任务ID:"+intToString(id));
        //取消该任务
        cancelTask(id);
    }
    conn->send(response);
}

void TaskManager::interListUndo(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=MSG_TIME_STRING_LEN *2){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        UserLogManager::getInstance()->push(conn->getUserName()+"请求排队任务列表");
        toDisMtx.lock();
        for(auto mmaptask: toDistributeTasks){
            for(auto task:mmaptask.second){
                TASK_INFO info;
                info.id = task->getId();
                sprintf_s(info.produceTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
                sprintf_s(info.doTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
                sprintf_s(info.doneTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
                info.excuteAgv = task->getAgv()->getId();
                info.status = AgvTask::AGV_TASK_STATUS_EXCUTING;
                memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(TASK_INFO));
                response.head.body_length = sizeof(TASK_INFO);
                response.head.flag = 1;
                conn->send(response);
            }
        }
        toDisMtx.unlock();
        response.head.flag = 0;
        response.head.body_length = 0;
    }
    conn->send(response);
}

void TaskManager::interListDoing(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=MSG_TIME_STRING_LEN *2){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        UserLogManager::getInstance()->push(conn->getUserName()+"请求正在执行任务列表");
        toDisMtx.lock();
        for(auto task: doingTask){
            TASK_INFO info;
            info.id = task->getId();
            sprintf_s(info.produceTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
            sprintf_s(info.doTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
            sprintf_s(info.doneTime,MSG_TIME_STRING_LEN,"%s",task->getProduceTime().c_str());
            info.excuteAgv = task->getAgv()->getId();
            info.status = AgvTask::AGV_TASK_STATUS_EXCUTING;
            memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(TASK_INFO));
            response.head.body_length = sizeof(TASK_INFO);
            response.head.flag = 1;
            conn->send(response);
        }
        toDisMtx.unlock();
        response.head.flag = 0;
        response.head.body_length = 0;
    }
    conn->send(response);
}

void TaskManager::interListDoneToday(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;


    std::stringstream ss;
    ss<<"select id,produce_time,do_time,done_time,agv from agv_task where done_time<= \'"<<getTimeStrTomorrow()<<"\' and done_time>= \'"<<getTimeStrToday()<<"\';";
    UserLogManager::getInstance()->push(conn->getUserName()+"请求今天完成任务列表");
    try{
        CppSQLite3Table table =  g_db.getTable(ss.str().c_str());
        for(int i=0;i<table.numRows();++i){
            table.setRow(i);
            TASK_INFO info;
            info.id = std::atoi( table.fieldValue(0));
            sprintf_s(info.produceTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(1));
            sprintf_s(info.doTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(2));
            sprintf_s(info.doneTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(3));
            info.excuteAgv = std::atoi( table.fieldValue(4));
            info.status = AgvTask::AGV_TASK_STATUS_DONE;
            memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(TASK_INFO));
            response.head.body_length = sizeof(TASK_INFO);
            response.head.flag = 1;
            conn->send(response);
        }
    }catch(CppSQLite3Exception e){
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
        LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
    }catch(std::exception e){
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
        LOG(ERROR)<<"sqlerr code:"<<e.what();
    }
    response.head.flag = 0;
    response.head.body_length = 0;

    conn->send(response);
}

void TaskManager::interListDuring(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=MSG_TIME_STRING_LEN *2){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        std::string startTime = std::string(msg.body);
        std::string endTime = std::string(msg.body+MSG_TIME_STRING_LEN);
        UserLogManager::getInstance()->push(conn->getUserName()+"查询历史任务，时间是从"+startTime+" 到"+endTime);
        std::stringstream ss;
        ss<<"select id,produce_time,do_time,done_time,agv from agv_task where done_time<= \'"<<endTime<<"\' and done_time>= \'"<<startTime<<"\';";

        try{
            CppSQLite3Table table =  g_db.getTable(ss.str().c_str());
            for(int i=0;i<table.numRows();++i){
                table.setRow(i);
                TASK_INFO info;
                info.id = std::atoi( table.fieldValue(0));
                sprintf_s(info.produceTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(1));
                sprintf_s(info.doTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(2));
                sprintf_s(info.doneTime,MSG_TIME_STRING_LEN,"%s",table.fieldValue(3));
                info.excuteAgv = std::atoi( table.fieldValue(4));
                info.status = AgvTask::AGV_TASK_STATUS_DONE;
                memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(TASK_INFO));
                response.head.body_length = sizeof(TASK_INFO);
                response.head.flag = 1;
                conn->send(response);
            }
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
        response.head.flag = 0;
        response.head.body_length = 0;
    }
    conn->send(response);
}
