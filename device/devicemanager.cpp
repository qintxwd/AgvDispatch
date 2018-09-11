#include "devicemanager.h"
#include "device.h"
#include "sqlite3/CppSQLite3.h"
#include "common.h"
#include "userlogmanager.h"
#include "Dongyao/charge/chargemachine.h"
#include "mapmap/mapmanager.h"
DeviceManager::DeviceManager()
{
}

bool DeviceManager::init()
{
    try{
        CppSQLite3Table table_device = g_db.getTable("select id, name, ip, port,lineID, type from agv_station where port != 0;");
        if (table_device.numRows() > 0 && table_device.numFields() != 6)return false;
        std::unique_lock<std::mutex> lck(mtx);
        for (int row = 0; row < table_device.numRows(); row++)
        {
            table_device.setRow(row);

            if (table_device.fieldIsNull(0) || table_device.fieldIsNull(1) || table_device.fieldIsNull(2) || table_device.fieldIsNull(3) || table_device.fieldIsNull(4))return false;
            int station = atoi(table_device.fieldValue(0));
            std::string name = std::string(table_device.fieldValue(1));
            std::string ip = std::string(table_device.fieldValue(2));
            int port = atoi(table_device.fieldValue(3));
            int deviceID = atoi(table_device.fieldValue(4));
            int deviceType = atoi(table_device.fieldValue(5));


            if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO && DEVICE_CHARGEMACHINE == deviceType)
            {
                chargemachinePtr device(new chargemachine());
                device->setParams(deviceID, name, ip, port, COMM_TIMEOUT);
                device->setType(deviceType);
                device->init();
                devices.push_back(device);
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

DevicePtr DeviceManager::getDeviceById(int id, int type)
{
    for(auto device:devices){
        if (device->getId() == id && device->getType() == type)
            return device;
    }

    return nullptr;
}

void DeviceManager::foreachDevice(DeviceEachCallback cb)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto device:devices){
        cb(device);
    }
}

void DeviceManager::getDeviceLog(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    int stationid = request["stationid"].asInt();

    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(stationid);
    //
    if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
    {
        response["result"] = RETURN_MSG_RESULT_FAIL;
        conn->send(response);
        return;
    }
    MapPoint *point = static_cast<MapPoint *>(spirit);
    int id = stoi(point->getLineId());
    int type = point->getPointType();

    if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO && DEVICE_CHARGEMACHINE == type)
    {
        DevicePtr device = getDeviceById(id, type);
        if(device != nullptr)
        {
            chargemachinePtr cm = std::static_pointer_cast<chargemachine>(device);
            bool ret = cm->chargeQueryList(id, response);
            if(!ret)
            {
                response["result"] = RETURN_MSG_RESULT_FAIL;
            }
            conn->send(response);
        }
        else
        {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            conn->send(response);
        }
    }
}

DevicePtr DeviceManager::getDeviceByIP(std::string ip)
{
    for(auto device:devices){
        if (device->getIp() == ip)
            return device;
    }
    return NULL;
}


void DeviceManager::interElevatorControl(SessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    int groupid = request["groupid"].asInt();
    int status = request["status"].asInt();

    auto group = MapManager::getInstance()->getGroupById(groupid);
    if(group != nullptr){
        combined_logger->info("group id:{0} change status to {1}", groupid, status);

        if(status == 0)
        {
            //停用电梯
           bool ret = MapManager::getInstance()->addOccuGroup(groupid, INT_MAX);
           if(!ret)
           {
               response["result"] = RETURN_MSG_RESULT_FAIL;
           }
        }
        else
        {
            //启用电梯 恢复锁定
            MapManager::getInstance()->freeGroup(groupid, INT_MAX);

        }
    }
    conn->send(response);
}
