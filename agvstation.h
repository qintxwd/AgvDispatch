#ifndef AGVSTATION_H
#define AGVSTATION_H
#include <string>
class Agv;
//Agv可到达和停留的站点
class AgvStation
{
public:
    AgvStation():
        x(0),
        y(0),
        name(""),
        id(0),
        occuAgv(nullptr),
        mapChangeStation(false)
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
    std::string name;
    Agv *occuAgv;
    bool mapChangeStation;//地图切换点
};

#endif // AGVSTATION_H
