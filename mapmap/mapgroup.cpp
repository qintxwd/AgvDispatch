#include "mapgroup.h"

MapGroup::MapGroup(int _id, std::string _name) :
    MapSpirit(_id,_name,Map_Sprite_Type_Group)
{

}

MapGroup::~MapGroup()
{

}

MapSpirit *MapGroup::clone()
{
    MapGroup *p = new MapGroup(getId(),getName());
    for(auto a:agvs){
        p->addAgv(a);
    }
    for(auto s:spirits){
        p->addSpirit(s);
    }
    return p;
}

