#include "qunchuangnodethingget.h"
#include "../agvImpl/ros/agv/rosAgv.h"

QunChuangNodeThingGet::QunChuangNodeThingGet(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{

}
void QunChuangNodeThingGet::beforeDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);

    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV 取料, beforeDoing...." << std::endl;

    string ip = station->getIp();
    if(ip.size() > 0)
    {
        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            std::cout << "AGV 取空卡塞, beforeDoing, ip: " <<ip<< std::endl;

        if(IsValidIPAddress(ip.c_str()))
        {
            std::cout << "AGV 取空卡塞, call ros_agv->beforeDoing " <<std::endl;

            ros_agv->beforeDoing(ip, station->getPort(),AGV_ACTION_GET, getStationNum(station->getName()));
        }
        else
        {
            combined_logger->error("QunChuangNodeThingGet::beforeDoing, IP {1} is invalid", ip);
            std::cout << "AGV 取料, beforeDoing... ip is invalid: "<< ip<<std::endl;
            return;
        }
    }
    else
    {
        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            combined_logger->info("AGV 取玻璃, no ip, do nothing");
    }
}

void QunChuangNodeThingGet::doing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV GET, doing...., station name:"<<station->getName() << std::endl;
    std::cout << "AGV GET, doing...., station id:"<<station->getId() << std::endl;
    std::cout << "AGV GET, doing...., station ip:"<<station->getIp() << std::endl;
    std::cout << "AGV GET, doing...., station port:"<<station->getPort() << std::endl;

    string ip = station->getIp();
    if(ip.size() > 0)
    {
        if(ros_agv->getAgvType() == Agv::AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
            std::cout << "AGV 取空卡塞, doing, ip: " <<ip<< std::endl;

        if(IsValidIPAddress(ip.c_str()))
        {
            ros_agv->Doing(AGV_ACTION_GET,  HexStringToInt(getStationNum(station->getName())));
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
            combined_logger->info("AGV 取玻璃, no ip, do nothing");
    }


}


void QunChuangNodeThingGet::afterDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);

    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

    std::cout << "AGV 取料, doing...." << std::endl;

    ros_agv->afterDoing(AGV_ACTION_GET,  HexStringToInt(getStationNum(station->getName())));

}

bool QunChuangNodeThingGet::result(AgvPtr agv)
{
    return bresult;
}
