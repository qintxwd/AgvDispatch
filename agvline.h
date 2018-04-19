#ifndef AGVLINE_H
#define AGVLINE_H
#include <vector>
#include <memory>

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

class AgvLine;
using AgvLinePtr = std::shared_ptr<AgvLine>;
//站点之间的连线
class AgvLine : public std::enable_shared_from_this<AgvLine>
{
public:
    AgvLine():
        id(0),
        length(0),
        father(0),
        distance(0),
        color(0)
    {

    }

    AgvLine(const AgvLine &b)
    {
        id = b.id;
        length = b.length;
        startStation = b.startStation;
        endStation = b.endStation;
    }
    bool operator <(const AgvLine &b) {
        return id<b.id;
    }

    bool operator == (const AgvLine &b) {
        return this->startStation == b.startStation&& this->endStation == b.endStation;
    }

    std::vector<AgvPtr> occuAgvs;

    int id;
    double length;
    AgvStationPtr startStation = nullptr;//起始站点
    AgvStationPtr endStation = nullptr;//终止站点

    //计算路径用的
    AgvLinePtr father;
    int distance;//起点到这条线的终点 的距离
    int color;

};

#endif // AGVLINE_H
