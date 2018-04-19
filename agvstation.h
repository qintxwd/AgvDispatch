#ifndef AGVSTATION_H
#define AGVSTATION_H
#include <string>
#include <memory>

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

//Agv可到达和停留的站点
class AgvStation : public std::enable_shared_from_this<AgvStation>
{
public:    
    AgvStation():
        x(0),
        y(0),
        name(""),
        id(0),
        occuAgv(nullptr),
        mapChangeStation(false),
        floorId(0)
    {
    }

    AgvStation(const AgvStation &b)
    {
        x = b.x;
        y = b.y;
        name = b.name;
        id = b.id;
        occuAgv = b.occuAgv;
        mapChangeStation = b.mapChangeStation;
        floorId = b.floorId;
    }

    bool operator <(const AgvStation &b) {
        return id<b.id;
    }
    bool operator == (const AgvStation &b) {
        return this->id == b.id;
    }

    int id;
    int x;
    int y;
    int floorId;//
    std::string name;
    AgvPtr occuAgv;
    bool mapChangeStation;//地图切换点
};

#endif // AGVSTATION_H
