#include "msgprocess.h"
#include "network/session.h"
#include "network/sessionmanager.h"
#include "usermanager.h"
#include "mapmap/mapmanager.h"
#include "agvmanager.h"
#include "userlogmanager.h"
#include "taskmanager.h"

MsgProcess::MsgProcess()
{

}

void MsgProcess::interAddSubAgvPosition(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " sub AGV position info");
    addSubAgvPosition(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubAgvStatus(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " sub AGV status info");
    addSubAgvStatus(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubTask(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " sub task info");
    addSubTask(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubLog(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " sub log info");
    addSubLog(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubAgvPosition(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " cancel AGV position info");
    removeSubAgvPosition(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubAgvStatus(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " cancel AGV status info");
    removeSubAgvStatus(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubTask(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " cancel task info");
    removeSubTask(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubLog(SessionPtr conn, const Json::Value &request) {
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName() + " cancel sub log");
    removeSubLog(conn->getSessionID());
    conn->send(response);
}

void MsgProcess::onSessionClosed(int id)
{
    removeSubAgvPosition(id);
    removeSubAgvStatus(id);
    removeSubTask(id);
    removeSubLog(id);
}

void MsgProcess::addSubAgvPosition(int id)
{
    UNIQUE_LCK(psMtx);
    if (std::find(agvPositionSubers.begin(), agvPositionSubers.end(), id) == agvPositionSubers.end()) {
        agvPositionSubers.push_back(id);
    }
}
void MsgProcess::addSubAgvStatus(int id)
{
    UNIQUE_LCK(ssMtx);
    if (std::find(agvStatusSubers.begin(), agvStatusSubers.end(), id) == agvStatusSubers.end()) {
        agvStatusSubers.push_back(id);
    }
}
void MsgProcess::addSubTask(int id)
{
    UNIQUE_LCK(tsMtx);
    if (std::find(taskSubers.begin(), taskSubers.end(), id) == taskSubers.end()) {
        taskSubers.push_back(id);
    }
}
void MsgProcess::addSubLog(int id)
{
    UNIQUE_LCK(lsMtx);
    if (std::find(logSubers.begin(), logSubers.end(), id) == logSubers.end()) {
        logSubers.push_back(id);
    }
}

void MsgProcess::removeSubAgvPosition(int id)
{
    UNIQUE_LCK(psMtx);
    auto itr = std::find(agvPositionSubers.begin(), agvPositionSubers.end(), id);
    if (itr != agvPositionSubers.end()) {
        agvPositionSubers.erase(itr);
    }
}
void MsgProcess::removeSubAgvStatus(int id)
{
    UNIQUE_LCK(ssMtx);
    auto itr = std::find(agvStatusSubers.begin(), agvStatusSubers.end(), id);
    if (itr != agvStatusSubers.end()) {
        agvStatusSubers.erase(itr);
    }
}
void MsgProcess::removeSubTask(int id)
{
    UNIQUE_LCK(tsMtx);
    auto itr = std::find(taskSubers.begin(), taskSubers.end(), id);
    if (itr != taskSubers.end()) {
        taskSubers.erase(itr);
    }
}
void MsgProcess::removeSubLog(int id)
{
    UNIQUE_LCK(lsMtx);
    auto itr = std::find(logSubers.begin(), logSubers.end(), id);
    if (itr != logSubers.end()) {
        logSubers.erase(itr);
    }
}

//100ms一次，发布AGV的位置
void MsgProcess::publisher_agv_position()
{
    const int position_pub_interval = 200;//100ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= position_pub_interval) {
            beginTime = endTime;
            //获取位置信息
            Json::Value aps;
            aps["type"] = MSG_TYPE_PUBLISH;
            aps["todo"] = MSG_TODO_PUB_AGV_POSITION;
            aps["queuenumber"] = 0;

            //AgvManager::getInstance()->getPositionJson(aps);

            if (agvPositionSubers.empty())continue;
            AgvManager::getInstance()->getPositionJson(aps);
            if(aps["agvs"].isNull())continue;
            //执行发送
            UNIQUE_LCK(psMtx);
            for (auto c : agvPositionSubers) {
                SessionManager::getInstance()->sendSessionData(c, aps);

            }
        }
    }
}

//每200ms发布一次小车状态
void MsgProcess::publisher_agv_status()
{
    const int status_pub_interval = 500;//200ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= status_pub_interval) {
            beginTime = endTime;
            //组装信息
            Json::Value response;
            //TODO
            if (agvStatusSubers.empty())continue;
            //执行发送
            //			UNIQUE_LCK(ssMtx);
            //			for (auto c : agvStatusSubers) {
            //				SessionManager::getInstance()->sendSessionData(c, response);
            //			}
        }
    }
}

//每500ms发布一次 当前任务的状态
void MsgProcess::publisher_task()
{
    const int task_pub_interval = 1000;//500ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= task_pub_interval) {
            beginTime = endTime;

            //组装信息
            Json::Value response;
            response["type"] = MSG_TYPE_PUBLISH;
            response["todo"] = MSG_TODO_PUB_TASK;
            response["queuenumber"] = 0;

            auto tasks = TaskManager::getInstance()->getCurrentTasks();
            if(tasks.size()<=0)continue;
            Json::Value v_tasks;
            for (auto task : tasks) {
                Json::Value v_task;
                v_task["id"] = task->getId();
                v_task["agv"] = task->getAgv();
                v_task["priority"] = task->getPriority();
                v_task["status"] = task->getStatus();
                v_task["produceTime"] = task->getProduceTime();
                v_task["doTime"] = task->getDoTime();
                v_task["doneTime"] = task->getDoneTime();
                v_task["cancelTime"] = task->getCancelTime();
                v_task["doingIndex"] = task->getDoingIndex();
                v_task["errorCode"] = task->getErrorCode();
                v_task["errorInfo"] = task->getErrorInfo();
                v_task["errorTime"] = task->getErrorTime();
                v_task["isCancel"] = task->getIsCancel();

                Json::Value v_extraParams;
                auto params = task->getExtraParams();
                for (auto param : params) {
                    v_extraParams[param.first] = param.second;
                }
                if (!v_extraParams.isNull()) {
                    v_task["extraParams"] = v_extraParams;
                }
                auto nodes = task->getTaskNodes();
                Json::Value v_nodes;
                for (auto node : nodes) {
                    Json::Value v_node;

                    auto things = node->getDoThings();

                    Json::Value v_things;
                    for (auto thing : things) {
                        Json::Value v_thing;
                        v_thing["id"] = thing->type() - AgvTaskNodeDoThing::Type;
                        v_thing["name"] = thing->discribe();
                        Json::Value v_params;
                        auto pparams = thing->getParams();
                        int kk = 0;
                        for (auto pparam : pparams) {
                            v_params[kk] = pparam;
                        }
                        if(!v_params.isNull())
                            v_thing["params"] = v_params;

                        v_things.append(v_thing);
                    }

                    v_node["station"] = node->getStation();
                    v_node["things"] = v_things;
                    v_nodes.append(v_node);
                }
                v_task["nodes"] = v_nodes;
                v_tasks.append(v_task);
            }
            response["tasks"] = v_tasks;

            //执行发送
            UNIQUE_LCK(tsMtx);
            if (taskSubers.size() <= 0)continue;
            for (auto c : taskSubers) {
                SessionManager::getInstance()->sendSessionData(c, response);

            }
        }
    }
}

bool MsgProcess::init()
{
    //启动3个pulish的线程
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_agv_position, this));
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_agv_status, this));
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_task, this));
    //日志发布时每次产生一个日志，发布一个日志
    return true;
}

void MsgProcess::sessionLogout(int user_id)
{
    //设置用户登录状态为
    std::stringstream ss;
    ss << "update agv_user set user_status=1 where id= " << user_id;
    g_db.execDML(ss.str().c_str());
}

void MsgProcess::removeSubSession(int session)
{
    psMtx.lock();
    agvPositionSubers.remove(session);
    psMtx.unlock();
    ssMtx.lock();
    agvStatusSubers.remove(session);
    ssMtx.unlock();
    tsMtx.lock();
    taskSubers.remove(session);
    tsMtx.unlock();
    lsMtx.lock();
    logSubers.remove(session);
    lsMtx.unlock();
}

//进来一个消息,分配给一个线程去处理它
void MsgProcess::processOneMsg(const Json::Value &request, SessionPtr session)
{
    //request需要copy一个到线程中。
    g_threadPool.enqueue([&, request,session] {

        combined_logger->info("RECV! session id={0}  len= {1}  json=\n{2}", session->getSessionID(), request.toStyledString().length(), request.toStyledString());

        //处理消息，如果有返回值，发送返回值
        //		if ((session->getUserId() <= 0 || session->getUserRole() <= USER_ROLE_VISITOR)) {
        //			if (request["todo"].asInt() != MSG_TODO_USER_LOGIN) {
        //				//未登录，却发送了 登录以外的 其它请求
        //				Json::Value response;
        //				response["type"] = MSG_TYPE_RESPONSE;
        //				response["todo"] = request["todo"];
        //				response["queuenumber"] = request["queuenumber"];
        //				response["result"] = RETURN_MSG_RESULT_FAIL;
        //				response["error_code"] = RETURN_MSG_ERROR_CODE_NOT_LOGIN;
        //				session->send(response);
        //				return;
        //			}
        //		}

        typedef std::function<void(SessionPtr, const Json::Value &)> ProcessFunction;

        //TODO:
        UserManagerPtr userManager = UserManager::getInstance();
        MapManagerPtr mapManager = MapManager::getInstance();
        AgvManagerPtr agvManager = AgvManager::getInstance();
        MsgProcessPtr msgProcess = shared_from_this();
        UserLogManagerPtr userLogManager = UserLogManager::getInstance();
        TaskManagerPtr taskManager = TaskManager::getInstance();

        static struct
        {
            MSG_TODO t;
            ProcessFunction f;
        } table[] =
        {
        { MSG_TODO_USER_LOGIN,std::bind(&UserManager::interLogin,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_LOGOUT,std::bind(&UserManager::interLogout,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_CHANGED_PASSWORD,std::bind(&UserManager::interChangePassword,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_LIST,std::bind(&UserManager::interList,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_DELTE,std::bind(&UserManager::interRemove,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_ADD,std::bind(&UserManager::interAdd,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_USER_MODIFY,std::bind(&UserManager::interModify,userManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_SET_MAP,std::bind(&MapManager::interSetMap,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_GET_MAP,std::bind(&MapManager::interGetMap,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_AGV_MANAGE_LIST,std::bind(&AgvManager::interList,agvManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_AGV_MANAGE_ADD,std::bind(&AgvManager::interAdd,agvManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_AGV_MANAGE_DELETE,std::bind(&AgvManager::interDelete,agvManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_AGV_MANAGE_MODIFY,std::bind(&AgvManager::interModify,agvManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_CREATE,std::bind(&TaskManager::interCreate,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_QUERY_STATUS,std::bind(&TaskManager::interQueryStatus,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_CANCEL,std::bind(&TaskManager::interCancel,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_LIST_UNDO,std::bind(&TaskManager::interListUndo,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_LIST_DOING,std::bind(&TaskManager::interListDoing,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_LIST_DONE_TODAY,std::bind(&TaskManager::interListDoneToday,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TASK_LIST_DURING,std::bind(&TaskManager::interListDuring,taskManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_LOG_LIST_DURING,std::bind(&UserLogManager::interLogDuring,userLogManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_AGV_POSITION,std::bind(&MsgProcess::interAddSubAgvPosition,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_AGV_POSITION,std::bind(&MsgProcess::interRemoveSubAgvPosition,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_AGV_STATSU,std::bind(&MsgProcess::interAddSubAgvStatus,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_AGV_STATSU,std::bind(&MsgProcess::interRemoveSubAgvStatus,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_LOG,std::bind(&MsgProcess::interAddSubLog,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_LOG,std::bind(&MsgProcess::interRemoveSubLog,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_TASK,std::bind(&MsgProcess::interAddSubTask,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_TASK,std::bind(&MsgProcess::interRemoveSubTask,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TRAFFIC_CONTROL_STATION,std::bind(&MapManager::interTrafficControlStation,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TRAFFIC_CONTROL_LINE,std::bind(&MapManager::interTrafficControlLine,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TRAFFIC_RELEASE_STATION,std::bind(&MapManager::interTrafficReleaseStation,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_TRAFFIC_RELEASE_LINE,std::bind(&MapManager::interTrafficReleaseLine,mapManager,std::placeholders::_1,std::placeholders::_2) },
    };
        table[request["todo"].asInt()].f(session, request);
    });
}


void MsgProcess::publishOneLog(USER_LOG log)
{
    //异步发布
    g_threadPool.enqueue([&, log] {
        if (logSubers.empty())return;

        Json::Value response;
        response["type"] = MSG_TYPE_PUBLISH;
        response["todo"] = MSG_TODO_PUB_LOG;
        response["queuenumber"] = 0;
        Json::Value vlog;
        vlog["time"] = log.time;
        vlog["msg"] = log.msg;
        response["log"] = vlog;
        UNIQUE_LCK(lsMtx);
        for (auto c : logSubers) {
            SessionManager::getInstance()->sendSessionData(c, response);
        }
    });
}

void MsgProcess::errorOccur(int code, std::string msg, bool needConfirm)
{
    errorMtx.lock();
    error_code = code;
    error_info = msg;
    needConfirm = needConfirm;
    errorMtx.unlock();
    notifyAll(ENUM_NOTIFY_ALL_TYPE_ERROR);
}

void MsgProcess::notifyAll(ENUM_NOTIFY_ALL_TYPE type)
{
    if (type == ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE) {

        Json::Value response;
        response["type"] = MSG_TYPE_NOTIFY;
        response["todo"] = MSG_TODO_NOTIFY_ALL_MAP_UPDATE;
        response["queuenumber"] = 0;

        SessionManager::getInstance()->sendData(response);
    }
    else if (type == ENUM_NOTIFY_ALL_TYPE_ERROR) {

        Json::Value response;
        response["type"] = MSG_TYPE_NOTIFY;
        response["todo"] = MSG_TODO_NOTIFY_ALL_ERROR;
        response["queuenumber"] = 0;
        response["needConfirm"] = needConfirm;
        response["error_code"] = error_code;
        response["error_info"] = error_info;

        SessionManager::getInstance()->sendData(response);
    }
}





