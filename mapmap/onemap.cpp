#include "onemap.h"

OneMap::OneMap():
    max_id(0)
{

}

OneMap::~OneMap()
{
    for(auto f:all_element)delete f;
}

void OneMap::clear()
{
    for(auto f:all_element)delete f;
    all_element.clear();
    max_id = 0;
}

void OneMap::addSpirit(MapSpirit *spirit)
{
    all_element.push_back(spirit);
}

void OneMap::removeSpirit(MapSpirit *spirit)
{
    all_element.remove(spirit);
}

void OneMap::removeSpiritById(int id)
{
    for(auto itr = all_element.begin();itr!=all_element.end();){
        if((*itr)->getId() == id){
            itr = all_element.erase(itr);
        }else{
            itr++;
        }
    }
}

int OneMap::getNextId()
{
    return ++max_id;
}

//复制整个地图
OneMap *OneMap::clone()
{
    OneMap *onemap = new OneMap;
    onemap->setMaxId(max_id);
    for (auto e : all_element) {
        onemap->addSpirit(e->clone());
    }
    return onemap;
}

MapSpirit *OneMap::getSpiritById(int id)
{
    for (auto p : all_element) {
        if(p->getId() == id)return p;
    }
    return nullptr;
}

std::list<MapFloor *> OneMap::getFloors()
{
    std::list<MapFloor *> floors;
    for(auto s:all_element)
    {
        if(s->getSpiritType() == MapSpirit::Map_Sprite_Type_Floor)floors.push_back(static_cast<MapFloor *>(s));
    }
    return floors;
}

std::list<MapPath *> OneMap::getPaths()
{
    std::list<MapPath *> paths;
    for(auto s:all_element)
    {
        if(s->getSpiritType() == MapSpirit::Map_Sprite_Type_Path)paths.push_back(static_cast<MapPath *>(s));
    }
    return paths;
}

std::list<MapPath *> OneMap::getRootPaths()
{
    std::list<MapPath *> all_paths = getPaths();
    std::list<MapFloor *> floors = getFloors();//楼层
    for(auto itr=all_paths.begin();itr!=all_paths.end();){
        //是否楼层上的路径
        bool isFloorPath = false;
        for(auto f:floors){
            if(isFloorPath)break;
            std::list<int> fps = f->getPaths();
            for(auto fp:fps){
                if((*itr)->getId() == fp){
                    isFloorPath = true;
                    break;
                }
            }
        }
        if(isFloorPath){
            itr = all_paths.erase(itr);
        }else{
            itr++;
        }
    }
    return all_paths;
}

std::list<MapBlock *> OneMap::getBlocks()
{
    std::list<MapBlock *> blocks;
    for(auto s:all_element)
    {
        if(s->getSpiritType() == MapSpirit::Map_Sprite_Type_Block)
        {
            blocks.push_back(static_cast<MapBlock *>(s));
        }
    }
    return blocks;
}

std::list<MapGroup *> OneMap::getGroups()
{
    std::list<MapGroup *> groups;
    for(auto s:all_element)
    {
        if(s->getSpiritType() == MapSpirit::Map_Sprite_Type_Group)
        {
            groups.push_back(static_cast<MapGroup *>(s));
        }
    }
    return groups;
}

