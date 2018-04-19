#include "agvmanager.h"
#include "agv.h"
#include "sqlite3/CppSQLite3.h"
#include "Common.h"

AgvManager* AgvManager::p = new AgvManager();

AgvManager::AgvManager()
{
}

void AgvManager::checkTable()
{
    //检查表
    try{
        CppSQLite3DB db;
        db.open(DB_File);
        if(!db.tableExists("agv_agv")){
            db.execDML("create table agv_agv(id int, name char(64),ip char(64),port int);");
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
        CppSQLite3DB db;
        db.open(DB_File);
        CppSQLite3Table table_agv = db.getTable("select id,name,ip,port from agv_agv;");
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
            Agv *agv = new Agv(id,name,ip,port);
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

void AgvManager::addAgv(Agv *agv)
{
    removeAgv(agv);
    std::unique_lock<std::mutex> lck(mtx);
    agvs.push_back(agv);
}

void AgvManager::removeAgv(Agv *agv)
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
