#include "mapfloor.h"

MapFloor::MapFloor(int _id, std::string _name):
    MapSpirit(_id,_name,Map_Sprite_Type_Floor),
    bkg(nullptr)
{
}

MapFloor::~MapFloor()
{
    if(bkg)delete bkg;
    for(auto itr = points.begin();itr!=points.end();++itr){
        delete *itr;
    }

    for(auto itr = paths.begin();itr!=paths.end();++itr){
        delete *itr;
    }
}

//复制地图（深copy）
MapFloor* MapFloor::clone()
{
    MapFloor *f = new MapFloor(getId(),getName());
    for(auto p : points) {
        MapPoint *newp = new MapPoint(*p);
        f->addPoint(newp);
    }

    for (auto p:paths) {
        MapPath *newp = new MapPath(*p);
        f->addPath(newp);
    }

    if(bkg){
        f->setBkg(bkg->clone());
    }
    return f;
}

MapPoint *MapFloor::getPointById(int id)
{
    for (auto p : points) {
        if(p->getId() == id)return p;
    }
    return nullptr;
}

MapPath *MapFloor::getPathById(int id)
{
    for (auto p : paths) {
        if(p->getId() == id)return p;
    }
    return nullptr;
}
