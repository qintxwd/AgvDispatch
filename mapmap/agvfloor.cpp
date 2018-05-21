#include "agvfloor.h"

AgvFloor::AgvFloor(int _id, std::string _name):
    MapFloor(_id,_name)
{

}

AgvFloor::AgvFloor(const AgvFloor &b):
    MapFloor(b)
{

}

AgvFloor::~AgvFloor()
{

}
