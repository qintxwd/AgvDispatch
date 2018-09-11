#include "mapgroup.h"

MapGroup::MapGroup(int _id, std::string _name, int _type) :
    MapSpirit(_id,_name,Map_Sprite_Type_Group)
{
    groupType = _type;
}

MapGroup::~MapGroup()
{

}

MapSpirit *MapGroup::clone()
{
    MapGroup *p = new MapGroup(getId(),getName(), getGroupType());
    for(auto s:spirits){
        p->addSpirit(s);
    }
    return p;
}

