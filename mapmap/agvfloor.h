#ifndef AGVFLOOR_H
#define AGVFLOOR_H

#include <memory>
#include "onemap.h"

class AgvFloor;
using AgvFloorPtr = std::shared_ptr<AgvFloor>;

class AgvFloor: public std::enable_shared_from_this<AgvFloor>, public MapFloor
{
public:
    AgvFloor(int _id, std::string _name);
    AgvFloor(const AgvFloor &b);
    virtual ~AgvFloor();
};

#endif // AGVFLOOR_H
