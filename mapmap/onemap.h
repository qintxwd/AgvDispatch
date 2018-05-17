#ifndef ONEMAP_H
#define ONEMAP_H

#include <list>

#include "mapfloor.h"
#include "mapbackground.h"
#include "mapblock.h"
#include "mapgroup.h"


//一个地图[有若干个元素组成]
class OneMap
{
public:
    OneMap();
    OneMap(const OneMap &b);

    ~OneMap();

    void clear();

    //注册一个新的元素 获取一个ID
    int getNextId();

    void addPath(MapPath *path);

    void addFloor(MapFloor *floor);

    void addBlock(MapBlock *block);

    void addGroup(MapGroup *group);

    //删除一个元素
    void removePath(MapPath *path);
    void removeFloor(MapFloor *floor);
    void removeBlock(MapBlock *block);
    void removeGroup(MapGroup *group);

    void removeRootPathById(int id);
    void removeFloorById(int id);

    //复制地图（深copy）
    OneMap* clone();

    std::list<MapFloor *> getFloors(){return floors;}
    std::list<MapPath *> getRootPaths(){return rootpaths;}
    std::list<MapBlock *> getBlocks(){return blocks;}
    std::list<MapGroup *> getGroups(){return groups;}

    MapFloor *getFloorById(int id);
    MapPath *getRootPathById(int id);
    MapBlock *getBlockById(int id);
    MapGroup *getGroupById(int id);

    MapSpirit *getSpiritById(int id);

    void setMaxId(int maxid){max_id = maxid; }
private:
    std::list<MapFloor *> floors;///楼层
    std::list<MapPath *> rootpaths;///楼层之间的道路
    std::list<MapBlock *> blocks;////block
    std::list<MapGroup *> groups;////groups
    int max_id;
};

#endif // ONEMAP_H
