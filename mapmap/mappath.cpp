#include "mappath.h"

MapPath::MapPath(int _id, std::string _name, int _start, int _end, Map_Path_Type _type, int _length, int _p1x, int _p1y, int _p2x, int _p2y, bool _locked):
    MapSpirit(_id,_name,Map_Sprite_Type_Path),
    start(_start),
    end(_end),
    length(_length),
    path_type(_type),
    p1x(_p1x),
    p1y(_p1y),
    p2x(_p2x),
    p2y(_p2y),
    locked(_locked)
{
}

MapPath::~MapPath()
{

}

MapSpirit *MapPath::clone()
{
    MapPath *p = new MapPath(getId(),getName(),getStart(),getEnd(),getPathType(),getLength(),getP1x(),getP1y(),getP2x(),getP2y(),getLocked());
    return p;
}

