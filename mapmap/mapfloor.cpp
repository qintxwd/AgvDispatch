#include "mapfloor.h"

MapFloor::MapFloor(int _id, std::string _name):
    MapSpirit(_id,_name,Map_Sprite_Type_Floor),
    bkg(0)
{
}

MapFloor::~MapFloor()
{

}

MapSpirit *MapFloor::clone()
{
    MapFloor *f = new MapFloor(getId(),getName());
    for(auto p : points) {
        f->addPoint(p);
    }
    for (auto p:paths) {
        f->addPath(p);
    }
    f->setBkg(bkg);

    return f;
}


