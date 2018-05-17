#include "mappath.h"

MapPath::MapPath(int _id, std::string _name, int _start, int _end, Map_Path_Type _type, int _length, int _p1x, int _p1y, int _p2x, int _p2y, bool _locked, int _direction):
    MapSpirit(_id,_name,Map_Sprite_Type_Path),
    start(_start),
    end(_end),
    length(_length),
    path_type(_type),
    p1x(_p1x),
    p1y(_p1y),
    p2x(_p2x),
    p2y(_p2y),
    locked(_locked),
    direction(_direction)
{
}

MapPath::MapPath(const MapPath &p):
    MapSpirit(p),
    start(p.start),
    end(p.end),
    path_type(p.path_type),
    length(p.length),
    p1x(p.p1x),
    p1y(p.p1y),
    p2x(p.p2x),
    p2y(p.p2y)
{
}
