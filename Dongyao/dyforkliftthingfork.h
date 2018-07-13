#ifndef DYFORKLIFTTHINGFORK_H
#define DYFORKLIFTTHINGFORK_H

#include "../agvtasknodedothing.h"

class DyForkliftThingFork : public AgvTaskNodeDoThing
{
public:
    DyForkliftThingFork(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 1 };

    int type(){return Type;}


    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("FORKLIFT FORK");
    }

private:
    int forkParams; //11-liftup 00-setdown
    bool bresult;//执行结果

};

#endif // DYFORKLIFTTHINGFORK_H
