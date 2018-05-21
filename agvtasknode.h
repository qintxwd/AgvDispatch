#ifndef AGVTASKNODE_H
#define AGVTASKNODE_H

#include <vector>
#include <memory>
#include "mapmap/mapmanager.h"
#include "agvtasknodedothing.h"

class AgvTaskNode;
using AgvTaskNodePtr = std::shared_ptr<AgvTaskNode>;

//一个任务节点由[零个或者一个]目的地 和 到达目的地后要做的多个事情助成
class AgvTaskNode : public std::enable_shared_from_this<AgvTaskNode>
{
public:
    AgvTaskNode():
        aimStation(0)
    {
    }

    int getStation(){return aimStation;}
    void setStation(int station){aimStation = station;}

    std::vector<AgvTaskNodeDoThingPtr> getDoThings(){return doThings;}
    void setDoThings(std::vector<AgvTaskNodeDoThingPtr> _doThings){doThings=_doThings;}
    void push_backDoThing(AgvTaskNodeDoThingPtr doThing){doThings.push_back(doThing);}

private:
    int aimStation;
    std::vector<AgvTaskNodeDoThingPtr> doThings;
};

#endif // AGVTASKNODE_H
