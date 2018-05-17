#include "mapgroup.h"

MapGroup::MapGroup(int _id, std::string _name) :
    MapSpirit(_id,_name,Map_Sprite_Type_Group)
{

}


MapGroup::MapGroup(const MapGroup& b):
    MapSpirit(b),
    spirits(b.spirits),
    agvs(b.agvs)
{
}

void MapGroup::init(std::list<int> _spirits, std::list<int> _agvs)
{
    spirits = _spirits;
    agvs = _agvs;
}
