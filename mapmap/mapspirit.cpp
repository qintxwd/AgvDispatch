#include "mapspirit.h"

MapSpirit::MapSpirit(int _id, std::string _name, Map_Spirit_Type _spirit_type):
    spirit_type(_spirit_type),
    id(_id),
    name(_name)
{

}

MapSpirit::MapSpirit(const MapSpirit &s)
{
    spirit_type = s.spirit_type;
    id = s.id;
    name = s.name;
}
