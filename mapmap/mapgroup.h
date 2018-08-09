#ifndef MAPGROUP_H
#define MAPGROUP_H

#include <list>
#include "mapspirit.h"

//一个区块 区块内只能有一个车
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

private:
    std::list<int> spirits;//区块
};

#endif // MAPGROUP_H
