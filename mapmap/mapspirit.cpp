#include "mapspirit.h"

MapSpirit::MapSpirit(int _id, std::string _name, Map_Spirit_Type _spirit_type):
    spirit_type(_spirit_type),
    id(_id),
    name(_name)
{

}

MapSpirit::~MapSpirit()
{

}

MapSpirit *MapSpirit::clone()
{
    MapSpirit *s = new MapSpirit(this->getId(),this->getName(),this->getSpiritType());
    return s;
}
