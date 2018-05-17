#include "onemap.h"

OneMap::OneMap():
    max_id(0)
{

}

OneMap::OneMap(const OneMap &b)
{
    max_id = b.max_id;
    floors = b.floors;
    rootpaths = b.rootpaths;
}

OneMap::~OneMap()
{
    for(auto f:floors)delete f;
    for(auto f:rootpaths)delete f;
    for(auto f:blocks)delete f;
    for(auto f:groups)delete f;
}

void OneMap::clear()
{
    for(auto f:floors)delete f;
    for(auto f:rootpaths)delete f;
    for(auto f:blocks)delete f;
    for(auto f:groups)delete f;
    floors.clear();
    rootpaths.clear();
    blocks.clear();
    groups.clear();
    max_id = 0;
}

void OneMap::addPath(MapPath *path)
{
    rootpaths.push_back(path);
}

void OneMap::addFloor(MapFloor *floor)
{
    floors.push_back(floor);
}

void OneMap::addBlock(MapBlock *block)
{
    blocks.push_back(block);
}

void  OneMap::addGroup(MapGroup *group)
{
    groups.push_back(group);
}

void OneMap::removeBlock(MapBlock *block)
{
    blocks.remove(block);
}

void OneMap::removeGroup(MapGroup *group)
{
    groups.remove(group);
}

void OneMap::removePath(MapPath *path)
{
    rootpaths.remove(path);
}
void OneMap::removeFloor(MapFloor *floor)
{
    floors.remove(floor);
}

void OneMap::removeRootPathById(int id)
{
    for (auto p:rootpaths) {
        if(p->getId() == id){
            rootpaths.remove(p);
            break;
        }
    }
}
void OneMap::removeFloorById(int id)
{
    for (auto f : floors) {
        if(f->getId() == id){
            floors.remove(f);
            break;
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
    for (auto p : rootpaths) {
        onemap->addPath(new MapPath(*p));
    }
    for (auto f : floors) {
        onemap->addFloor(f->clone());
    }
    for (auto b : blocks) {
        onemap->addBlock(new MapBlock(*b));
    }
    for (auto b : groups) {
        onemap->addGroup(new MapGroup(*b));
    }
    return onemap;
}

MapFloor *OneMap::getFloorById(int id)
{
    for (auto p : floors) {
        if(p->getId() == id)return p;
    }

    return nullptr;
}

MapPath *OneMap::getRootPathById(int id)
{
    for (auto p : rootpaths) {
        if(p->getId() == id)return p;
    }

    return nullptr;
}

MapBlock *OneMap::getBlockById(int id)
{
    for (auto b : blocks) {
        if(b->getId() == id)return b;
    }
    return nullptr;
}

MapGroup *OneMap::getGroupById(int id)
{
    for (auto b : groups) {
        if(b->getId() == id)return b;
    }
    return nullptr;
}

MapSpirit *OneMap::getSpiritById(int id)
{
    MapFloor *f = getFloorById(id);
    if(f!=nullptr)return f;
    MapPath *p = getRootPathById(id);
    if(p)return p;
    MapBlock *b = getBlockById(id);
    if(b)return b;
    MapGroup *g = getGroupById(id);
    if(g)return g;

    for (auto floor : floors) {
        auto pos = floor->getPaths();
        auto pis = floor->getPoints();
        for (auto po : pos) {
            if(po->getId() == id)return po;
        }
        for (auto pi: pis) {
            if(pi->getId() == id)return pi;
        }
    }

    return nullptr;
}

