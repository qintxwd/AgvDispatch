#include "dymappath.h"

DyMapPath::DyMapPath(int _id, std::string _name, int _start, int _end, Map_DyPath_Type _type, int _p1x, int _p1y, int _p1a, int _p1f, int _p2x, int _p2y, int _p2a, int _p2f, float _speed):
    MapSpirit(_id,_name,Map_Sprite_Type_DyPath),
    start(_start),
    end(_end),
    path_type(_type),
    p1x(_p1x),
    p1y(_p1y),
    p1a(_p1a),
    p1f(_p1f),
    p2x(_p2x),
    p2y(_p2y),
    p2a(_p2a),
    p2f(_p2f),
    speed(_speed){
}

DyMapPath::~DyMapPath()
{

}

MapSpirit *DyMapPath::clone()
{
    DyMapPath *p = new DyMapPath(getId(),getName(),getStart(),getEnd(),getPathType(),getP1x(),getP1y(),getP1a(),getP1f(),getP2x(),getP2y(),getP2a(),getP2f());
    return p;
}


