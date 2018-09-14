#include "agvmanager.h"
#include "agv.h"
#include "sqlite3/CppSQLite3.h"
#include "common.h"
#include "userlogmanager.h"
#include "agvImpl/ros/agv/rosAgv.h"
#include "virtualrosagv.h"
#include "Dongyao/dyforklift.h"
#include "Anting/atforklift.h"

AgvManager::AgvManager()
{
}

void AgvManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_agv")){
            g_db.execDML("create table agv_agv(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER,lastStation INTEGER,nowStation INTEGER,nextStation INTEGER,agvType INTEGER,agvClass INTEGER, lineName char(64));");
        }
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return ;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}",e.what());
        return ;
    }
}

void AgvManager::setServerAccepterID(int serverID)
{
    _serverID = serverID;
}

bool AgvManager::init()
{
    checkTable();
    try{
        CppSQLite3Table table_agv = g_db.getTable("select id,name,ip,port,lastStation,nowStation,nextStation,agvType,agvClass,lineName from agv_agv;");
        if (table_agv.numRows() > 0 && table_agv.numFields() != 10)return false;
        std::unique_lock<std::mutex> lck(mtx);
        for (int row = 0; row < table_agv.numRows(); row++)
        {
            table_agv.setRow(row);

            if (table_agv.fieldIsNull(0) || table_agv.fieldIsNull(1) || table_agv.fieldIsNull(2) || table_agv.fieldIsNull(3) || table_agv.fieldIsNull(4) || table_agv.fieldIsNull(5) || table_agv.fieldIsNull(6))return false;
            int id = atoi(table_agv.fieldValue(0));
            std::string name = std::string(table_agv.fieldValue(1));
            std::string ip = std::string(table_agv.fieldValue(2));
            int port = atoi(table_agv.fieldValue(3));
            int lastStation = atoi(table_agv.fieldValue(4));
            int nowStation = atoi(table_agv.fieldValue(5));
            int nextStation = atoi(table_agv.fieldValue(6));

            int agvType = -1;
            int agvClass = -1;
            std::string lineName = "";

            if(!table_agv.fieldIsNull(7))
                agvType = atoi(table_agv.fieldValue(7));

            if(!table_agv.fieldIsNull(8))
                agvClass = atoi(table_agv.fieldValue(8));

            if(!table_agv.fieldIsNull(9))
                lineName = std::string(table_agv.fieldValue(9));

            if (GLOBAL_AGV_PROJECT == AGV_PROJECT_QUNCHUANG) // 群创
            {
                AgvPtr agv(new rosAgv(id,name,ip,port,agvType,agvClass,lineName));
                agv->init();
                agv->setType(agvType);
                agv->setPosition(lastStation, nowStation, nextStation);
                agvs.push_back(agv);
            }
            else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO)
            {
                if(1 == agvType)
                {
                    AgvPtr agv(new VirtualRosAgv(id, name));
                    agv->setType(agvType);
                    agv->status = Agv::AGV_STATUS_IDLE;
                    agv->init();
                    agv->setPosition(lastStation, nowStation, nextStation);
                    agvs.push_back(agv);
                }
                else
                {
                    DyForkliftPtr agv(new DyForklift(id, name, ip, port));
                    agv->setType(agvType);
                    agv->status = Agv::AGV_STATUS_UNCONNECT;
                    agv->setPosition(0,0,0);
                    agvs.push_back(agv);
                }
            }
            else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING)
            {
                if(AGV_TYPE_ANTING_FORKLIFT == agvType)
                {
                    AtForkliftPtr agv(new AtForklift(id, name, ip, port));
                    agv->setType(agvType);
                    agv->status = Agv::AGV_STATUS_NOTREADY;
                    agv->setPosition(0,0,0);
                    agvs.push_back(agv);
                }
            }
            else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_QINGDAO)
            {
                if(AGV_TYPE_VIRTUAL_ROS_AGV == agvType)
                {
                    AgvPtr agv(new VirtualRosAgv(id, name));
                    agv->setType(agvType);
                    agv->init();
                    agv->setPosition(lastStation, nowStation, nextStation);
                    agvs.push_back(agv);
                }
            }
        }
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return false;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}",e.what());
        return false;
    }
    return true;
}

void AgvManager::updateAgv(int id, std::string name, std::string ip, int port, int lastStation, int nowStation, int nextStation)
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
            agv->setPosition(lastStation, nowStation, nextStation);
        }
    }
}

AgvPtr AgvManager::getAgvById(int id)
{
    for(auto agv:agvs){
        if (agv->getId() == id)
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
        }
        else
            ++itr;
    }
}

void AgvManager::removeAgv(int agvId)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto itr = agvs.begin();itr!=agvs.end();){
        if((*itr)->getId() == agvId){
            itr = agvs.erase(itr);
        }
        else
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

void AgvManager::getPositionJson(Json::Value &json)
{
    auto mapmanagerptr = MapManager::getInstance();
    std::unique_lock<std::mutex> lck(mtx);
    Json::Value json_all_agv;
    for (auto agv : agvs) {
        bool online_flag = true;
        if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO || GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING)
        {
            if(!agv->getPort())
            {
                online_flag = false;
            }
        }

        if (GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING) {
            if (agv->status == Agv::AGV_STATUS_NOTREADY)
                continue;
        }

        if(online_flag)
        {
            Json::Value json_one_agv;
            json_one_agv["id"] = agv->getId();
            json_one_agv["name"] = agv->getName();
            json_one_agv["x"] = agv->getX();
            json_one_agv["y"] = agv->getY();
            json_one_agv["theta"] = agv->getTheta();
            
            //发布agv所属楼层ID
            json_one_agv["floor"] = agv->getFloor();
            json_one_agv["nowStation"] = agv->getNowStation();

            //发布占用道路和站点信息[用于绘制不同的线路和站点的颜色]
            auto sps = mapmanagerptr->getOccuSpirit(agv->getId());
            std::stringstream ss;
            for (auto sp : sps)ss << sp << ";";
            json_one_agv["occurs"] = ss.str();

            json_all_agv.append(json_one_agv);
        }
    }
    if (json_all_agv.size() > 0)
        json["agvs"] = json_all_agv;
}

void AgvManager::interList(SessionPtr conn, const Json::Value &request)
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
        info["lastStation"] = agv->getLastStation();
        info["nowStation"] = agv->getNowStation();
        info["nextStation"] = agv->getNextStation();
        agv_infos.append(info);
    }
    response["agvs"] = agv_infos;
    conn->send(response);
}

void AgvManager::interAdd(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    //TYPE...
    //station...
    //
    if (request["name"].isNull() ||
            request["ip"].isNull()||
            request["port"].isNull()||
            request["agv_type"].isNull() ||
            request["station"].isNull()) {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else {

        UserLogManager::getInstance()->push(conn->getUserName()+" add AGV.name:"+ request["name"].asString() +" ip:"+request["ip"].asString()+intToString(request["port"].asInt()));
        char buf[SQL_MAX_LENGTH];
        string lineName = "";
        snprintf(buf, SQL_MAX_LENGTH, "insert into agv_agv(name,ip,port,lastStation,nowStation,nextStation,agvType,agvClass,lineName) values('%s','%s',%d,%d,%d,0,%d,%d,'%s');", request["name"].asString().c_str(), request["ip"].asString().c_str(), request["port"].asInt(), request["station"].asInt(), request["station"].asInt(), -1, 0, lineName.c_str());
        try{
            g_db.execDML(buf);
            int id = g_db.execScalar("select max(id) from agv_agv;");
            response["id"] = id;

            int stationId = request["station"].asInt();
            if (request["agv_type"] == "virtual") {
                AgvPtr agv(new VirtualRosAgv(id, request["name"].asString()));
                agv->init();
                agv->setPosition(stationId, stationId, 0);
                addAgv(agv);
            }
            else {
                AgvPtr agv(new rosAgv(id, request["name"].asString(), request["ip"].asString(), request["port"].asInt()));
                agv->init();
                agv->setPosition(stationId, stationId, 0);
                addAgv(agv);
            }
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

void AgvManager::interDelete(SessionPtr conn, const Json::Value &request)
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

void AgvManager::interStop(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    int id = request["id"].asInt();
    int params = request["params"].asInt();

    //TODO 如果节点发生变化需下发给小车当前位置 AGVTYPE需要修正
    if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT)
    {
        AgvPtr agv = getAgvById(id);
        if(agv==nullptr && agv->type() == DyForklift::Type){
            DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);
            forklift->stopEmergency(params);
        }else{
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_UNKNOW;
        }
    }
    conn->send(response);
}


void AgvManager::interModify(SessionPtr conn, const Json::Value &request)
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
            //            request["type"].isNull() ||
            request["lastStation"].isNull() ||
            request["nowStation"].isNull() ||
            request["nextStation"].isNull()) {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
    }
    else {
        int id = request["id"].asInt();
        std::string name = request["name"].asString();
        int port = request["port"].asInt();
        std::string ip = request["ip"].asString();
        int lastStation = request["lastStation"].asInt();
        int nowStation = request["nowStation"].asInt();
        int nextStation = request["nextStation"].asInt();

        //TODO 如果节点发生变化需下发给小车当前位置 AGVTYPE需要修正
        AgvPtr agv = getAgvById(id);

        if(agv->getNowStation() != nowStation)
        {
            if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT && agv->type() == DyForklift::Type)
            {
                DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);
                forklift->setInitPos(nowStation);
            }
        }
        int agvType = -1;
        int agvClass = 0;
        std::string lineName = "";

        UserLogManager::getInstance()->push(conn->getUserName() + " modify AGV.ID:" + intToString(id) + " newname:" + name + " newip:" + ip + " newport:" + intToString(port)+ " lastStation:" + intToString(lastStation)+ " nowStation:" + intToString(nowStation) + " nextStation:" + intToString(nextStation));
        char buf[SQL_MAX_LENGTH];
        snprintf(buf, SQL_MAX_LENGTH, "update agv_agv set name='%s',ip='%s',port=%d,lastStation=%d,nowStation=%d,nextStation=%d,agvClass=%d,lineName='%s'  where id = %d;", name.c_str(), ip.c_str(), port, lastStation, nowStation, nextStation,agvClass,lineName,id);

        try{
            g_db.execDML(buf);
            updateAgv(id, std::string(name), std::string(ip), port, lastStation, nowStation, nextStation);
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


AgvPtr AgvManager::getAgvByIP(std::string ip)
{
    for(auto agv:agvs){
        if (agv->getIp() == ip)
            return agv;
    }
    return nullptr;
}
