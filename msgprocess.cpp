#include "msgprocess.h"
#include "network/tcpsession.h"

MsgProcess* MsgProcess::p = new MsgProcess();


//8个线程处理所有消息，应该足够了
MsgProcess::MsgProcess():
    pool(12)
{

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
            for(auto c = agvPositionSubers.begin();c!=agvPositionSubers.end();++c){
                for(auto m = msgs.begin();m!=msgs.end();++m){
                    (*c)->send(*m);
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
            for(auto c = agvStatusSubers.begin();c!=agvStatusSubers.end();++c){
                for(auto m = msgs.begin();m!=msgs.end();++m){
                    (*c)->send(*m);
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
            for(auto c = taskSubers.begin();c!=taskSubers.end();++c){
                for(auto m = msgs.begin();m!=msgs.end();++m){
                    (*c)->send(*m);
                }
            }
        }
    }
}

bool MsgProcess::init()
{
    //启动3个pulish的线程
    pool.enqueue(std::bind(&MsgProcess::publisher_agv_position,this));
    pool.enqueue(std::bind(&MsgProcess::publisher_agv_status,this));
    pool.enqueue(std::bind(&MsgProcess::publisher_task,this));
    //日志发布时每次产生一个日志，发布一个日志
    return true;
}

//进来一个消息,分配给一个线程去处理它
void MsgProcess::processOneMsg(MSG_Request request,qyhnetwork::TcpSessionPtr session)
{
    //request需要copy一个到线程中。
    pool.enqueue([&,request]{
        //处理消息，如果有返回值，发送返回值
        MSG_Response response;
        memcpy(&(response.head),&(request.head),sizeof(MSG_Head));
        response.return_head.error_code = 0;
        response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        response.head.body_length = 0;

        if(session->getUserId()<=0 && request.head.todo != MSG_TODO_USER_LOGIN){
            //未登录，却请求了其它请求
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_LOGIN;
            response.return_head.result = RETURN_MSG_RESULT_FAIL;
            session->send(response);
            return ;
        }

        typedef std::function<void(qyhnetwork::TcpSessionPtr, MSG_Request)> ProcessFunction;

        //TODO:
    });
}


void MsgProcess::publishOneLog(USER_LOG log)
{
    //异步发布
    pool.enqueue([&,log]{
        if(logSubers.empty())return ;

        MSG_Response response;
        memcpy(&response,0,sizeof(MSG_Response));
        response.head.head = 0x88;
        response.head.queuenumber = 0;
        response.head.tail = 0xAA;
        response.head.todo = MSG_TODO_PUB_LOG;
        memcpy(response.body,&log,sizeof(log));
        response.head.body_length = sizeof(log.time)+strlen(log.msg);

        for(auto c = logSubers.begin();c!=logSubers.end();++c){
            (*c)->send(response);
        }
    });
}




