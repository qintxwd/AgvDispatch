#include "agvmanager.h"
#include "agv.h"
#include "sqlite3/CppSQLite3.h"
#include "common.h"
#include "userlogmanager.h"


AgvManager::AgvManager()
{
}

void AgvManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_agv")){
            g_db.execDML("create table agv_agv(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER);");
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
            AgvPtr agv(new Agv(id,name,ip,port));
            agvs.push_back(agv);
        }
    }catch(CppSQLite3Exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        //return false;
    }catch(std::exception e){
        LOG(ERROR)<<"sqlerr code:"<<e.what();
        //return false;
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

AgvPtr AgvManager::getAgvById(int id)
{
    for(auto agv:agvs){
        if(agv->getId() == id)return agv;
    }
    return nullptr;
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

void AgvManager::interList(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

    UserLogManager::getInstance()->push(conn->getUserName()+"请求AGV列表");

    Json::Value agv_infos;
    UNIQUE_LCK lck(mtx);
    for (auto agv : agvs) {
        Json::Value info;
        info["id"] = agv->getId();
        info["ip"] = agv->getIp();
        info["name"] = agv->getName();
        info["port"] = agv->getPort();
        agv_infos.append(info);
    }
    response["agvs"] = agv_infos;
    conn->send(response);
}

void AgvManager::interAdd(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

    if(request["id"].isNull()||
            request["name"].isNull()||
            request["ip"].isNull()||
            request["port"].isNull()||
            request["type"].isNull()){
		response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+"添加AGV.name:"+ request["name"].asString() +" ip:"+request["ip"].asString()+intToString(request["port"].asInt()));
        char buf[SQL_MAX_LENGTH];
        snprintf(buf,SQL_MAX_LENGTH, "insert into agv_agv(name,ip,port) values('%s','%s',%d);", request["name"].asString().c_str(), request["ip"].asString().c_str(), request["port"].asInt());
        try{
            g_db.execDML(buf);
            int id = g_db.execScalar("select max(id) from agv_agv;");
			response["id"] = id;

            AgvPtr agv(new Agv(id, request["name"].asString(), request["ip"].asString(), request["port"].asInt()));
            agv->init();
            addAgv(agv);
        }
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.what();
		}
    }
    conn->send(response);
}

void AgvManager::interDelete(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

    //TODO:添加到数据库，获取ID返回
	if (request["id"].isNull()) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
    else {
        int id = request["id"].asInt();
        UserLogManager::getInstance()->push(conn->getUserName()+"删除AGV.ID:"+ intToString(id));
        char buf[SQL_MAX_LENGTH];
        snprintf(buf,SQL_MAX_LENGTH, "delete from agv_agv where id=%d;", id);
        try{
            g_db.execDML(buf);
            removeAgv(id);
        }
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.what();
		}
    }
    conn->send(response);
}

void AgvManager::interModify(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

    //TODO:添加到数据库，获取ID返回
	if (request["id"].isNull() ||
		request["name"].isNull() ||
		request["ip"].isNull() ||
		request["port"].isNull() ||
		request["type"].isNull()) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
    else {
		int id = request["name"].asInt();
		std::string name = request["name"].asString();
		int port = request["port"].asInt();
		std::string ip = request["ip"].asString();

        UserLogManager::getInstance()->push(conn->getUserName()+"修改AGV信息.ID:"+ intToString(id)+" newname:"+ name +" newip:"+ ip +" newport:"+intToString(port));
        char buf[SQL_MAX_LENGTH];
        snprintf(buf,SQL_MAX_LENGTH, "update agv_agv set name=%s,ip=%s,port=%d where id = %d;", name.c_str(), ip.c_str(),port,id);

        try{
            g_db.execDML(buf);
            updateAgv(id,std::string(name),std::string(ip),port);
        }
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.what();
		}
    }
    conn->send(response);
}
