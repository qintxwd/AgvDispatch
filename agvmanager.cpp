#include "agvmanager.h"
#include "agv.h"
#include "sqlite3/CppSQLite3.h"
#include "Common.h"
#include "userlogmanager.h"


AgvManager::AgvManager()
{
}

void AgvManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_agv")){
            g_db.execDML("create table agv_agv(id int primary key AUTO_INCREMENT, name char(64),ip char(64),port int);");
        }
    }catch(CppSQLite3Exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        return ;
    }catch(std::exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.what();
        return ;
    }
}


bool AgvManager::init()
{
    checkTable();
    try{
        CppSQLite3Table table_agv = g_db.getTable("select id,name,ip,port from agv_agv;");
        if(table_agv.numRows()>0 && table_agv.numFields()!=4)return false;
        std::unique_lock<std::mutex> lck(mtx);
        for (int row = 0; row < table_agv.numRows(); row++)
        {
            table_agv.setRow(row);

            if(table_agv.fieldIsNull(0) ||table_agv.fieldIsNull(1) ||table_agv.fieldIsNull(2)||table_agv.fieldIsNull(3))return false;
            int id = atoi(table_agv.fieldValue(0));
            std::string name = std::string(table_agv.fieldValue(1));
            std::string ip = std::string(table_agv.fieldValue(2));
            int port = atoi(table_agv.fieldValue(3));
            AgvPtr agv = AgvPtr(new Agv(id,name,ip,port));
            agvs.push_back(agv);
        }
    }catch(CppSQLite3Exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        return false;
    }catch(std::exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.what();
        return false;
    }
    return true;
}

void AgvManager::updateAgv(int id, std::string name, std::string ip, int port)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto agv:agvs){
        if(agv->getId() == id){
            agv->setName(name);
            if(agv->getIp()!=ip || agv->getPort()!=port){
                agv->setIp(ip);
                agv->setPort(port);
                agv->reconnect();
            }
        }
    }
}

void AgvManager::addAgv(AgvPtr agv)
{
    removeAgv(agv);
    std::unique_lock<std::mutex> lck(mtx);
    agvs.push_back(agv);
}

void AgvManager::removeAgv(AgvPtr agv)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto itr = agvs.begin();itr!=agvs.end();){
        if(*itr == agv){
            itr = agvs.erase(itr);
        }else
            ++itr;
    }
}

void AgvManager::removeAgv(int agvId)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto itr = agvs.begin();itr!=agvs.end();){
        if((*itr)->getId() == agvId){
            itr = agvs.erase(itr);
        }else
            ++itr;
    }
}

void AgvManager::foreachAgv(AgvEachCallback cb)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto agv:agvs){
        cb(agv);
    }
}

void AgvManager::interList(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    UserLogManager::getInstance()->push(conn->getUserName()+"请求AGV列表");
    UNIQUE_LCK lck(mtx);
    for (auto agv : agvs) {
        AGV_BASE_INFO info;
        info.id = agv->getId();
        sprintf_s(info.ip,MSG_STRING_LEN,"%s",agv->getIp().c_str());
        sprintf_s(info.name,MSG_STRING_LEN,"%s",agv->getName().c_str());
        info.port = agv->getPort();
        memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(AGV_BASE_INFO));
        response.head.flag = 1;
        response.head.body_length = sizeof(AGV_BASE_INFO);
        conn->send(response);
    }
    response.head.body_length = 0;
    response.head.flag = 0;
    conn->send(response);
}

void AgvManager::interAdd(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    //TODO:添加到数据库，获取ID返回
    if (msg.head.body_length != sizeof(AGV_BASE_INFO)) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error AGV_BASE_INFO length");
    }
    else {
        AGV_BASE_INFO baseinfo;
        memcpy_s(&baseinfo,sizeof(AGV_BASE_INFO), msg.body, sizeof(AGV_BASE_INFO));
        if (msg.head.body_length != sizeof(AGV_LINE)) {
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error AGV_LINE length");
        }

        UserLogManager::getInstance()->push(conn->getUserName()+"添加AGV.name:"+ baseinfo.name+" ip:"+baseinfo.ip+intToString(baseinfo.port));
        char buf[SQL_MAX_LENGTH];
        sprintf_s(buf,SQL_MAX_LENGTH, "insert into agv_agv(name,ip,port) values(%s,%s,%d);", baseinfo.name, baseinfo.ip,baseinfo.port);
        try{
            g_db.execDML(buf);
            int id = g_db.execScalar("select max(id) from agv_agv;");
            response.head.body_length = sizeof(int);
            memcpy(response.body,&id,sizeof(int));
            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;

            AgvPtr agv = AgvPtr(new Agv(id,std::string(baseinfo.name),std::string(baseinfo.ip),baseinfo.port));
            agv->init();
            addAgv(agv);
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(FATAL)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(FATAL)<<"sqlerr code:"<<e.what();
        }
    }
    conn->send(response);
}

void AgvManager::interDelete(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    //TODO:添加到数据库，获取ID返回
    if (msg.head.body_length != sizeof(int)) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }
    else {
        uint32_t id;
        memcpy(&id, msg.body, sizeof(uint32_t));
        UserLogManager::getInstance()->push(conn->getUserName()+"删除AGV.ID:"+ intToString(id));
        char buf[SQL_MAX_LENGTH];
        sprintf_s(buf,SQL_MAX_LENGTH, "delete from agv_agv where id=%d;", id);
        try{
            g_db.execDML(buf);
            removeAgv(id);
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(FATAL)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(FATAL)<<"sqlerr code:"<<e.what();
        }
    }
    conn->send(response);
}

void AgvManager::interModify(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy(&response.head, &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    //TODO:添加到数据库，获取ID返回
    if (msg.head.body_length != sizeof(AGV_BASE_INFO)) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }
    else {
        AGV_BASE_INFO baseinfo;
        memcpy(&baseinfo, msg.body, sizeof(AGV_BASE_INFO));
        UserLogManager::getInstance()->push(conn->getUserName()+"修改AGV信息.ID:"+ intToString(baseinfo.id)+" newname:"+ baseinfo.name+" newip:"+baseinfo.ip+" newport:"+intToString( baseinfo.port));
        char buf[SQL_MAX_LENGTH];
        sprintf_s(buf,SQL_MAX_LENGTH, "update agv_agv set name=%s,ip=%s,port=%d where id = %d;", baseinfo.name, baseinfo.ip,baseinfo.port,baseinfo.id);

        try{
            g_db.execDML(buf);
            updateAgv(baseinfo.id,std::string(baseinfo.name),std::string(baseinfo.ip),baseinfo.port);
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(FATAL)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(FATAL)<<"sqlerr code:"<<e.what();
        }
    }
    conn->send(response);
}
