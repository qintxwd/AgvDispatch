#ifndef ONEMAP_H
#define ONEMAP_H

#include <list>

#include "mapfloor.h"
#include "mapbackground.h"
#include "mapblock.h"
#include "mapgroup.h"


//一个地图[有若干个元素组成]//用于显示和保存地图
class OneMap
{
public:
    OneMap();
    OneMap(const OneMap &b) = delete;

    ~OneMap();

    void clear();

    //注册一个新的元素 获取一个ID
    int getNextId();

    void addSpirit(MapSpirit *spirit);

    //删除一个元素
    void removeSpirit(MapSpirit *spirit);
    void removeSpiritById(int id);

    //复制地图（深copy）
    OneMap* clone();

    std::list<MapFloor *> getFloors();
    std::list<MapPath *> getRootPaths();
    std::list<MapPath *> getPaths();
    std::list<MapBlock *> getBlocks();
    std::list<MapGroup *> getGroups();
    std::list<MapSpirit *> getAllElement(){return all_element;}

    MapSpirit *getSpiritById(int id);

    void setMaxId(int maxid){max_id = maxid; }
    int getMaxId(){return max_id; }
private:
    std::list<MapSpirit *> all_element;
    int max_id;
};

#endif // ONEMAP_H
