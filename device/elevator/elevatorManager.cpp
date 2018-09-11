#include "elevatorManager.h"
#include "sqlite3/CppSQLite3.h"
#include "common.h"
#include "userlogmanager.h"
#include "elevator.h"

ElevatorManager::ElevatorManager()
{
}

ElevatorManager::~ElevatorManager()
{

}

void ElevatorManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_elevator")){
            g_db.execDML("create table agv_elevator(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER,waittingpoints INTEGER,left BOOL,right BOOL,enabled BOOL);");
        }
    }catch(CppSQLite3Exception e){
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return ;
    }catch(std::exception e){
        combined_logger->error("sqlerr code:{0}",e.what());
        return ;
    }
}


bool ElevatorManager::init()
{
    checkTable();
    try{
        CppSQLite3Table table_ele = g_db.getTable("select id,name,ip,port,waittingpoints,left,right,enabled from agv_elevator;");
        if(table_ele.numRows()>0 && table_ele.numFields()!=8)
        {
            combined_logger->error("ElevatorManager init agv_elevator table error!");
            return false;
        }
        std::unique_lock<std::mutex> lck(mtx);
        for (int row = 0; row < table_ele.numRows(); row++)
        {
            table_ele.setRow(row);

            if(table_ele.fieldIsNull(7))
            {
                combined_logger->error("Elevator {0} enabled field is null", row);
                continue;
            }
            if(table_ele.fieldIsNull(0) ||table_ele.fieldIsNull(1) ||table_ele.fieldIsNull(2)||table_ele.fieldIsNull(3))
                return false;

            int id = atoi(table_ele.fieldValue(0));
            std::string name = std::string(table_ele.fieldValue(1));
            std::string ip = std::string(table_ele.fieldValue(2));
            int port = atoi(table_ele.fieldValue(3));

            std::string waitting_points = std::string(table_ele.fieldValue(4));

            bool left_enabled=false;
            bool right_enabled=false;
            bool ele_enabled=true;

            if(!table_ele.fieldIsNull(5))
                left_enabled = atoi(table_ele.fieldValue(5)) == 1;

            if(!table_ele.fieldIsNull(6))
                right_enabled = atoi(table_ele.fieldValue(6)) == 1;

            if(!table_ele.fieldIsNull(7))
                ele_enabled = atoi(table_ele.fieldValue(7)) == 1;

            ElevatorPtr ele(new Elevator(id,name,ip,port,left_enabled,right_enabled,ele_enabled,waitting_points));
            ele->init();
            ele->start();
            elevators.push_back(ele);
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



ElevatorPtr ElevatorManager::getElevatorById(int id)
{
    for(auto ele:elevators){
        if(ele->getId() == id)
            return ele;
    }
    return nullptr;
}



