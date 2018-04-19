#ifndef AGVTASKNODE_H
#define AGVTASKNODE_H

#include <vector>

#include "agvstation.h"
#include "agvtasknodedothing.h"

//一个任务节点由[零个或者一个]目的地 和 到达目的地后要做的多个事情助成
class AgvTaskNode
{
public:
    AgvTaskNode():
        aimStation(nullptr)
    {
    }

    AgvStationPtr getStation(){return aimStation;}
    void setStation(AgvStationPtr station){aimStation = station;}

    std::vector<AgvTaskNodeDoThing *> getDoThings(){return doThings;}
    void setDoThings(std::vector<AgvTaskNodeDoThing *> _doThings){doThings=_doThings;}
    void push_backDoThing(AgvTaskNodeDoThing *doThing){doThings.push_back(doThing);}

private:
    AgvStationPtr aimStation;
    std::vector<AgvTaskNodeDoThing *> doThings;
};

#endif // AGVTASKNODE_H
