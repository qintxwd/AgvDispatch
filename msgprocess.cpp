#include "msgprocess.h"
#include "network/tcpsession.h"
#include "network/sessionmanager.h"
#include "usermanager.h"
#include "mapmanager.h"
#include "agvmanager.h"
#include "userlogmanager.h"
#include "taskmanager.h"

MsgProcess::MsgProcess()
{

}

void MsgProcess::interAddSubAgvPosition(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"订阅AGV实时位置信息");
    addSubAgvPosition(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubAgvStatus(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"订阅AGV实时状态信息");
    addSubAgvStatus(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"订阅任务信息");
    addSubTask(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interAddSubLog(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"订阅日志信息");
    addSubLog(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubAgvPosition(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"取消订阅AGV实时位置信息");
    removeSubAgvPosition(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubAgvStatus(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"取消订阅AGV实时状态信息");
    removeSubAgvStatus(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"取消订阅任务信息");
    removeSubTask(conn->getSessionID());
    conn->send(response);
}
void MsgProcess::interRemoveSubLog(qyhnetwork::TcpSessionPtr conn, MSG_Request msg){
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"取消订阅日志信息");
    removeSubLog(conn->getSessionID());
    conn->send(response);
}

void MsgProcess::addSubAgvPosition(int id)
{
    UNIQUE_LCK(psMtx);
    if(std::find(agvPositionSubers.begin(),agvPositionSubers.end(),id)==agvPositionSubers.end()){
        agvPositionSubers.push_back(id);
    }
}
void MsgProcess::addSubAgvStatus(int id)
{
    UNIQUE_LCK(ssMtx);
    if(std::find(agvStatusSubers.begin(),agvStatusSubers.end(),id)==agvStatusSubers.end()){
        agvStatusSubers.push_back(id);
    }
}
void MsgProcess::addSubTask(int id)
{
    UNIQUE_LCK(tsMtx);
    if(std::find(taskSubers.begin(),taskSubers.end(),id)==taskSubers.end()){
        taskSubers.push_back(id);
    }
}
void MsgProcess::addSubLog(int id)
{
    UNIQUE_LCK(lsMtx);
    if(std::find(logSubers.begin(),logSubers.end(),id)==logSubers.end()){
        logSubers.push_back(id);
    }
}

void MsgProcess::removeSubAgvPosition(int id)
{
    UNIQUE_LCK(psMtx);
    auto itr = std::find(agvPositionSubers.begin(),agvPositionSubers.end(),id);
    if(itr!=agvPositionSubers.end()){
        agvPositionSubers.erase(itr);
    }
}
void MsgProcess::removeSubAgvStatus(int id)
{
    UNIQUE_LCK(ssMtx);
    auto itr = std::find(agvStatusSubers.begin(),agvStatusSubers.end(),id);
    if(itr!=agvStatusSubers.end()){
        agvStatusSubers.erase(itr);
    }
}
void MsgProcess::removeSubTask(int id)
{
    UNIQUE_LCK(tsMtx);
    auto itr = std::find(taskSubers.begin(),taskSubers.end(),id);
    if(itr!=taskSubers.end()){
        taskSubers.erase(itr);
    }
}
void MsgProcess::removeSubLog(int id)
{
    UNIQUE_LCK(lsMtx);
    auto itr = std::find(logSubers.begin(),logSubers.end(),id);
    if(itr!=logSubers.end()){
        logSubers.erase(itr);
    }
}

//100ms一次，发布AGV的位置
void MsgProcess::publisher_agv_position()
{
    const int position_pub_interval = 100;//100ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= position_pub_interval) {
            beginTime = endTime;
            //获取位置信息
            std::list<MSG_Response> msgs;//TODO
            //= AgvManager::getInstance()->getPositions();
            if(agvPositionSubers.empty())continue;
            if(msgs.empty())continue;
            //执行发送
            UNIQUE_LCK(psMtx);
            for(auto c :agvPositionSubers){
                for(auto m : msgs){
                    qyhnetwork::SessionManager::getInstance()->sendSessionData(c,m);
                }
            }
        }
    }
}

//每200ms发布一次小车状态
void MsgProcess::publisher_agv_status()
{
    const int status_pub_interval = 200;//200ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= status_pub_interval) {
            beginTime = endTime;
            //组装信息
            std::list<MSG_Response> msgs;// = AgvManager::getInstance()->getstatuss();
            if(agvStatusSubers.empty())continue;
            if(msgs.empty())continue;
            //执行发送
            UNIQUE_LCK(ssMtx);
            for(auto c : agvStatusSubers){
                for(auto m : msgs){
                    qyhnetwork::SessionManager::getInstance()->sendSessionData(c,m);
                }
            }
        }
    }
}

//每500ms发布一次 当前任务的状态
void MsgProcess::publisher_task()
{
    const int task_pub_interval = 500;//500ms
    std::chrono::high_resolution_clock::time_point beginTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
        if (interval.count() >= task_pub_interval) {
            beginTime = endTime;

            //组装信息
            std::list<MSG_Response> msgs;
            //TODO

            //执行发送
            UNIQUE_LCK(tsMtx);
            for(auto c : taskSubers){
                for(auto m : msgs){
                    qyhnetwork::SessionManager::getInstance()->sendSessionData(c,m);
                }
            }
        }
    }
}

bool MsgProcess::init()
{
    //启动3个pulish的线程
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_agv_position,this));
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_agv_status,this));
    g_threadPool.enqueue(std::bind(&MsgProcess::publisher_task,this));
    //日志发布时每次产生一个日志，发布一个日志
    return true;
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
void MsgProcess::processOneMsg(MSG_Request request,qyhnetwork::TcpSessionPtr session)
{
    //request需要copy一个到线程中。
    g_threadPool.enqueue([&,request]{
        //处理消息，如果有返回值，发送返回值
        if((session->getUserId()<=0 || session->getUserRole()<=USER_ROLE_VISITOR) && request.head.todo != MSG_TODO_USER_LOGIN){
            //未登录，却发送了 登录以外的 其它请求
            MSG_Response response;
            memcpy(&(response.head),&(request.head),sizeof(MSG_Head));
            response.return_head.error_code = 0;
            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
            response.head.body_length = 0;
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_LOGIN;
            response.return_head.result = RETURN_MSG_RESULT_FAIL;
            session->send(response);
            return ;
        }

        typedef std::function<void(qyhnetwork::TcpSessionPtr, MSG_Request)> ProcessFunction;

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

        { MSG_TODO_MAP_CREATE_START,std::bind(&MapManager::interCreateStart,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_CREATE_ADD_STATION,std::bind(&MapManager::interCreateAddStation,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_CREATE_ADD_LINE,std::bind(&MapManager::interCreateAddLine,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_CREATE_FINISH,std::bind(&MapManager::interCreateFinish,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_LIST_STATION,std::bind(&MapManager::interListStation,mapManager,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_MAP_LIST_LINE,std::bind(&MapManager::interListLine,mapManager,std::placeholders::_1,std::placeholders::_2) },

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

        //订阅类
        { MSG_TODO_SUB_AGV_POSITION,std::bind(&MsgProcess::interAddSubAgvPosition,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_AGV_POSITION,std::bind(&MsgProcess::interRemoveSubAgvPosition,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_AGV_STATSU,std::bind(&MsgProcess::interAddSubAgvStatus,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_AGV_STATSU,std::bind(&MsgProcess::interRemoveSubAgvStatus,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_LOG,std::bind(&MsgProcess::interAddSubLog,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_LOG,std::bind(&MsgProcess::interRemoveSubLog,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_SUB_TASK,std::bind(&MsgProcess::interAddSubTask,msgProcess,std::placeholders::_1,std::placeholders::_2) },
        { MSG_TODO_CANCEL_SUB_TASK,std::bind(&MsgProcess::interRemoveSubTask,msgProcess,std::placeholders::_1,std::placeholders::_2) }

    };
        table[request.head.todo].f(session, request);
    });
}


void MsgProcess::publishOneLog(USER_LOG log)
{
    //异步发布
    g_threadPool.enqueue([&,log]{
        if(logSubers.empty())return ;

        MSG_Response response;
        memcpy(&response,0,sizeof(MSG_Response));
        response.head.head = 0x88;
        response.head.queuenumber = 0;
        response.head.flag = 0;
        response.head.tail = 0xAA;
        response.head.todo = MSG_TODO_PUB_LOG;
        memcpy_s(response.body,MSG_LONG_STRING_LEN,&log,sizeof(log));
        response.head.body_length = sizeof(log.time)+strlen(log.msg);
        UNIQUE_LCK(lsMtx);
        for(auto c:logSubers){
            qyhnetwork::SessionManager::getInstance()->sendSessionData(c,response);
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
    if(type == ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE){
        MSG_Response response;
        memcpy(&response,0,sizeof(MSG_Response));
        response.head.head = 0x88;
        response.head.queuenumber = 0;
        response.head.flag = 0;
        response.head.tail = 0xAA;
        response.head.todo = MSG_TODO_NOTIFY_ALL_MAP_UPDATE;

        qyhnetwork::SessionManager::getInstance()->sendData(response);
    }else if(type == ENUM_NOTIFY_ALL_TYPE_ERROR){
        MSG_Response response;
        memcpy(&response,0,sizeof(MSG_Response));
        response.head.head = 0x88;
        response.head.queuenumber = 0;
        response.head.flag = 0;
        response.head.tail = 0xAA;
        response.head.todo = MSG_TODO_NOTIFY_ALL_ERROR;
        NOTIFY_ERROR info;
        errorMtx.lock();
        info.needConfirm = needConfirm;
        info.code = error_code;
        sprintf_s(info.msg,MSG_RESPONSE_BODY_MAX_SIZE-4-sizeof(bool),"%s",error_info.c_str());
        memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(NOTIFY_ERROR));
        response.head.body_length = sizeof(int32_t)+sizeof(bool)+error_info.length();
        errorMtx.unlock();
        qyhnetwork::SessionManager::getInstance()->sendData(response);
    }
}



