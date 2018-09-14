#include "mapfloor.h"

MapFloor::MapFloor(int _id, std::string _name):
    MapSpirit(_id,_name,Map_Sprite_Type_Floor),
    bkg(0),
    originX(0),//原点位置的 对应位置
    originY(0),//原点位置的 对应位置
    rate(1.0)//agv的距离（实际距离） 和 地图位置的比例
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
    f->setOriginX(originX);
    f->setOriginY(originY);
	f->setOriginTheta(originTheta);
    return f;
}


