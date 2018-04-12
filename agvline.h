#ifndef AGVLINE_H
#define AGVLINE_H
#include <vector>
class AgvStation;
class Agv;
//站点之间的连线
class AgvLine
{
public:
    AgvLine():
        id(0),
        length(0),
        father(0),
        distance(0),
        color(0),
        startStation(NULL),
        endStation(NULL)
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

    std::vector<Agv *> occuAgvs;

    int id;
    double length;
    AgvStation* startStation;//起始站点
    AgvStation* endStation;//终止站点

    //计算路径用的
    AgvLine* father;
    int distance;//起点到这条线的终点 的距离
    int color;

};

#endif // AGVLINE_H
