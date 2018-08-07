#include "qunchuangnodedoing.h"
#include "../agvImpl/ros/agv/rosAgv.h"

QunChuangNodeDoing::QunChuangNodeDoing(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{

}
void QunChuangNodeDoing::beforeDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);

    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

 
}

void QunChuangNodeDoing::doing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

 
}


void QunChuangNodeDoing::afterDoing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);

    rosAgvPtr ros_agv = std::static_pointer_cast<rosAgv>(agv);
    int station_id = this->getStationId();

    auto mapSpirit = MapManager::getInstance()->getMapSpiritById(station_id);
    MapPoint *station = static_cast<MapPoint *>(mapSpirit);

}

bool QunChuangNodeDoing::result(AgvPtr agv)
{
    return bresult;
}
