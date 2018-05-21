#ifndef MAPGROUP_H
#define MAPGROUP_H

#include <list>
#include "mapspirit.h"

//一个区块，只允许部分车辆通行
class MapGroup : public MapSpirit
{
public:
    explicit MapGroup(int _id, std::string _name);
    virtual ~MapGroup();
    virtual MapSpirit *clone();
    MapGroup(const MapGroup& b) = delete;

    void addSpirit(int spirit){spirits.push_back(spirit);}
    void removeSpirit(int spirit){spirits.remove(spirit);}
    std::list<int> getSpirits(){return spirits;}

    void addAgv(int agv){agvs.push_back(agv);}
    void removeAgv(int agv){agvs.remove(agv);}
    std::list<int> getAgvs()  const {return agvs;}
private:
    std::list<int> spirits;//区块
    std::list<int> agvs;//通行车辆
};

#endif // MAPGROUP_H
