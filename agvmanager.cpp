#include "agvmanager.h"
#include "agv.h"
#include "sqlite3/CppSQLite3.h"
#include "common.h"
#include "userlogmanager.h"
#include "agvImpl/ros/agv/rosAgv.h"


AgvManager::AgvManager()
{
}

void AgvManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_agv")){
            g_db.execDML("create table agv_agv(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER,agvType INTEGER,agvClass INTEGER, lineName char(64));");
        }
    }catch(CppSQLite3Exception e){
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return ;
    }catch(std::exception e){
        combined_logger->error("sqlerr code:{0}",e.what());
        return ;
    }
}


bool AgvManager::init()
{
    checkTable();
    try{
        CppSQLite3Table table_agv = g_db.getTable("select id,name,ip,port,agvType,agvClass,lineName from agv_agv;");
        if(table_agv.numRows()>0 && table_agv.numFields()!=7)
        {
            combined_logger->error("AgvManager init agv_agv table error!");
            return false;
        }
        std::unique_lock<std::mutex> lck(mtx);
        for (int row = 0; row < table_agv.numRows(); row++)
        {
            table_agv.setRow(row);

            if(table_agv.fieldIsNull(0) ||table_agv.fieldIsNull(1) ||table_agv.fieldIsNull(2)||table_agv.fieldIsNull(3)||table_agv.fieldIsNull(4)||table_agv.fieldIsNull(5)||table_agv.fieldIsNull(6))return false;
            int id = atoi(table_agv.fieldValue(0));
            std::string name = std::string(table_agv.fieldValue(1));
            std::string ip = std::string(table_agv.fieldValue(2));
            int port = atoi(table_agv.fieldValue(3));
            int agvType = atoi(table_agv.fieldValue(4));
            int agvClass = atoi(table_agv.fieldValue(5));
            std::string lineName = std::string(table_agv.fieldValue(6));


            //AgvPtr agv(new Agv(id,name,ip,port));

            if(GLOBAL_AGV_PROJECT == AGV_PROJECT_QUNCHUANG) // 群创
            {
                AgvPtr agv(new rosAgv(id,name,ip,port,agvType,agvClass,lineName));
                agv->init();
                agvs.push_back(agv);
            }
            else
            {
                AgvPtr agv(new Agv(id,name,ip,port));
                agv->init();
                agvs.push_back(agv);
            }

        }
    }catch(CppSQLite3Exception e){
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return false;
    }catch(std::exception e){
        combined_logger->error("sqlerr code:{0}",e.what());
        return false;
    }
    return true;
}

void AgvManager::updateAgv(int id, std::string name, std::string ip, int port, int agvType, int agvClass, std::string lineName)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto agv:agvs){
        if(agv->getId() == id){
            agv->setName(name);
            if(agv->getIp()!=ip || agv->getPort()!=port){
                agv->setIp(ip);
                agv->setPort(port);
                agv->reconnect();
                /*agv->setAgvType(agvType);
                agv->setAgvClass(agvClass);
                agv->setLineName(lineName);
                */
            }
        }
    }
}

AgvPtr AgvManager::getAgvById(int id)
{
    for(auto agv:agvs){
        if(agv->getId() == id)
            return agv;
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

    UserLogManager::getInstance()->push(conn->getUserName()+" list agv");

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

    if(request["name"].isNull()||
            request["ip"].isNull()||
            request["port"].isNull()||
            request["type"].isNull()/*||
            request["agvType"].isNull()||
            request["agvClass"].isNull()||
            request["lineName"].isNull()*/){
		response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+" add AGV.name:"+ request["name"].asString() +" ip:"+request["ip"].asString()+intToString(request["port"].asInt()));
        char buf[SQL_MAX_LENGTH];
        string lineName = "";
        snprintf(buf,SQL_MAX_LENGTH, "insert into agv_agv(name,ip,port,agvType,agvClass,lineName) values('%s','%s',%d,%d,%d,'%s');", request["name"].asString().c_str(), request["ip"].asString().c_str(), request["port"].asInt(), -1, 0, lineName.c_str());
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
            combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(), e.errorMessage());
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} ",e.what());
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
        UserLogManager::getInstance()->push(conn->getUserName()+" delete AGV.ID:"+ intToString(id));
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
            combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(), e.errorMessage());
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} ",e.what());
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
        /*int agvType = request["agvType"].asInt();
        int agvClass = request["agvClass"].asInt();
        std::string lineName = request["lineName"].asString();*/
        int agvType = -1;
        int agvClass = 0;
        std::string lineName = "";

        UserLogManager::getInstance()->push(conn->getUserName()+" modify AGV.ID:"+ intToString(id)+" newname:"+ name +" newip:"+ ip +" newport:"+intToString(port));
        char buf[SQL_MAX_LENGTH];
        snprintf(buf,SQL_MAX_LENGTH, "update agv_agv set name=%s,ip=%s,port=%d,agvType=%d,agvClass=%d,lineName=%s where id = %d;", name.c_str(), ip.c_str(),port,agvType,agvClass,lineName,id);

        try{
            g_db.execDML(buf);
            updateAgv(id,std::string(name),std::string(ip),port,agvType,agvClass,lineName);
        }
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(), e.errorMessage());
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} ",e.what());
		}
    }
    conn->send(response);
}
