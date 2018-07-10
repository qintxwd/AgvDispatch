#include "qunchuangnodetingput.h"
#include "../agvImpl/ros/agv/rosAgv.h"

QunChuangNodeTingPut::QunChuangNodeTingPut(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{

}

void QunChuangNodeTingPut::beforeDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV PUT, beforeDoing...., station name:"<<station->getName() << std::endl;
    std::cout << "AGV PUT, beforeDoing...., station id:"<<station->getId() << std::endl;
    std::cout << "AGV PUT, beforeDoing...., station ip:"<<station->getIp() << std::endl;
    std::cout << "AGV PUT, beforeDoing...., station port:"<<station->getPort() << std::endl;

    string ip = station->getIp();
    if(ip.size() > 0)
    {
        std::cout << "AGV 上料到偏贴机, beforeDoing, ip.size() > 0 " << std::endl;

        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            std::cout << "AGV 上料到偏贴机, beforeDoing, ip: " <<ip<< std::endl;

        if(IsValidIPAddress(ip.c_str()))
        {
            std::cout << "AGV 上料到偏贴机, call ros_agv  beforeDoing" << std::endl;

            ros_agv->beforeDoing(ip, station->getPort(),AGV_ACTION_PUT,  HexStringToInt(getStationNum(station->getName())));
        }
        else
        {
            combined_logger->error("QunChuangNodeThingGet::beforeDoing, IP {1} is invalid", ip);
            return;
        }
    }
    else
    {
        combined_logger->info("AGV 上料到偏贴机, no ip, do nothing");
    }
}

void QunChuangNodeTingPut::doing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV PUT, doing...., station name:"<<station->getName() << std::endl;
    std::cout << "AGV PUT, doing...., station id:"<<station->getId() << std::endl;
    std::cout << "AGV PUT, doing...., station ip:"<<station->getIp() << std::endl;
    std::cout << "AGV PUT, doing...., station port:"<<station->getPort() << std::endl;


    string ip = station->getIp();
    if(ip.size() > 0)
    {
        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            std::cout << "AGV 上料到偏贴机, doing, ip: " <<ip<< std::endl;

        if(IsValidIPAddress(ip.c_str()))
        {
            ros_agv->Doing(AGV_ACTION_PUT,  HexStringToInt(getStationNum(station->getName())));
        }
        else
        {
            combined_logger->error("QunChuangNodeThingGet::beforeDoing, IP {1} is invalid", ip);
            return;
        }
    }
    else
    {
        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            combined_logger->info("AGV 上料到偏贴机, no ip, do nothing");
    }

}


void QunChuangNodeTingPut::afterDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);

    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV QunChuangNodeTingPut, afterDoing....." << std::endl;

    ros_agv->afterDoing(AGV_ACTION_PUT,  HexStringToInt(getStationNum(station->getName())));

    std::cout << "AGV QunChuangNodeTingPut, afterDoing end....." << std::endl;


}

bool QunChuangNodeTingPut::result(AgvPtr agv)
{
    return bresult;
}
