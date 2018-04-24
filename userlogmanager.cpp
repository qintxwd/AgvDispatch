#include "userlogmanager.h"
#include "msgprocess.h"


UserLogManager::UserLogManager()
{

}

void UserLogManager::checkTable(){
    if(!g_db.tableExists("agv_log")){
        g_db.execDML("create table agv_log(id INTEGER primary key AUTOINCREMENT, log_time,log_msg char(1024));");
    }
}


void UserLogManager::init()
{
    checkTable();

    g_threadPool.enqueue([&]{
        while(true){
            if(logQueue.empty())
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            mtx.lock();
            USER_LOG log = logQueue.front();
            logQueue.pop();
            mtx.unlock();

            //1.存库
            try{
                std::stringstream ss;
                ss<<"insert into agv_log (log_time,log_msg) values ("<<log.time<<" ,"<<log.msg<<" );";
                g_db.execDML(ss.str().c_str());
            }catch(CppSQLite3Exception &e){
                LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
            }catch(std::exception e){
                LOG(ERROR) << e.what();
            }
            //2.发布
            MsgProcess::getInstance()->publishOneLog(log);
        }
    });
}

void UserLogManager::push(const std::string &s)
{
    std::string time = getTimeStrNow();
    USER_LOG log;
    snprintf(log.time,MSG_TIME_STRING_LEN,"%s", time.c_str());
    snprintf(log.msg,MSG_LOG_MAX_LENGTH,"%s", s.c_str());
    mtx.lock();
    logQueue.push(log);
    mtx.unlock();
}

void UserLogManager::interLogDuring(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if(msg.head.body_length!=MSG_TIME_STRING_LEN *2){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else{
        std::string startTime = std::string(msg.body);
        std::string endTime = std::string(msg.body+MSG_TIME_STRING_LEN);
        push(conn->getUserName()+"查询历史日志，时间是从"+startTime+" 到"+endTime);
        try{
            std::stringstream ss;
            ss<<"select log_time,log_msg from agv_log where log_time >= \'"<<startTime<<"\' and log_time<=\'"<<endTime<<"\' ;";
            CppSQLite3Table table = g_db.getTable(ss.str().c_str());
            if(table.numRows()>0 && table.numFields() == 2 ){
                for(int i=0;i<table.numRows();++i){
                    table.setRow(i);
                    USER_LOG log;
                    snprintf(log.time,MSG_TIME_STRING_LEN, table.fieldValue(0));
                    snprintf(log.msg,MSG_LOG_MAX_LENGTH, table.fieldValue(1));
                    snprintf(response.body,MSG_RESPONSE_BODY_MAX_SIZE,"%s",log.time);
                    snprintf(response.body+MSG_TIME_STRING_LEN,MSG_LOG_MAX_LENGTH,"%s",log.msg);
                    response.head.body_length = MSG_TIME_STRING_LEN+strlen(log.msg);
					response.head.flag = 1;
                    conn->send(response);
                }
				response.head.body_length = 0;
				response.head.flag = 0;
            }
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }

    conn->send(response);
}
