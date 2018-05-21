#include "mappoint.h"

MapPoint::MapPoint(int _id, std::string _name, Map_Point_Type _type, int _x, int _y, int _realX, int _realY, int _labelXoffset, int _labelYoffset, bool _mapChange, bool _locked):
    MapSpirit(_id,_name,Map_Sprite_Type_Point),
    x(_x),
    y(_y),
    labelXoffset(_labelXoffset),
    labelYoffset(_labelYoffset),
    mapChange(_mapChange),
    locked(_locked),
    realX(_realX),
    realY(_realY),
    point_type(_type)
{
}

MapPoint::~MapPoint()
{
}


MapSpirit *MapPoint::clone()
{
    MapPoint *p = new MapPoint(this->getId(),this->getName(),this->getPointType(),this->getX(),this->getY(),this->getRealX(),this->getRealY(),this->getLabelXoffset(),this->getLabelYoffset(),this->getMapChange(),this->getLocked());
    return p;
}
