#include "userlogmanager.h"
#include "msgprocess.h"


UserLogManager::UserLogManager():
    logQueue(64)
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

    g_threadPool.enqueue([this]{
        while(true){

            if (logQueue.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                continue;
            }

            USER_LOG log;
            logQueue.pop(log);

            //1.存库
            try{
                std::stringstream ss;
                ss<<"insert into agv_log (log_time,log_msg) values ('"<<log.time<<"' ,'"<<log.msg<<"' );";
                g_db.execDML(ss.str().c_str());
            }catch(CppSQLite3Exception &e){
                std::cerr << e.errorCode() << ":" << e.errorMessage();
            }catch(std::exception e){
                std::cerr << e.what();
            }
            //2.发布
            MsgProcess::getInstance()->publishOneLog(log);
        }
    });
}

void UserLogManager::push(const std::string &s)
{
    USER_LOG log;
    snprintf(log.time,LOG_TIME_LENGTH,"%s",getTimeStrNow().c_str());
    snprintf(log.msg,LOG_MSG_LENGTH,"%s",s.c_str());
    logQueue.push(log);
}

void UserLogManager::interLogDuring(SessionPtr conn,const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
    if (request["startTime"].isNull() ||
            request["endTime"].isNull()) {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }else{
        std::string startTime = request["startTime"].asString();
        std::string endTime = request["endTime"].asString();
        push(conn->getUserName()+"query history log，time from"+startTime+" to"+endTime);
        Json::Value agv_logs;
        try{
            std::stringstream ss;
            ss<<"select log_time,log_msg from agv_log where log_time >= \'"<<startTime<<"\' and log_time<=\'"<<endTime<<"\' ;";
            CppSQLite3Table table = g_db.getTable(ss.str().c_str());
            if(table.numRows()>0 && table.numFields() == 2 ){
                for(int i=0;i<table.numRows();++i){
                    table.setRow(i);
                    Json::Value agv_log;
                    agv_log["time"] = std::string(table.fieldValue(0));
                    agv_log["msg"] = std::string(table.fieldValue(1));
                    agv_logs.append(agv_log);
                }
            }
            response["logs"] = agv_logs;
        }
        catch (CppSQLite3Exception e) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
            response["error_info"] = ss.str();
            combined_logger->error( "sqlerr code:{0} msg:{1}" , e.errorCode(), e.errorMessage()) ;
        }
        catch (std::exception e) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "info:" << e.what();
            response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0}" ,e.what()) ;
        }
    }

    conn->send(response);
}
