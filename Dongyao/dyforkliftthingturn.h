#ifndef DYFORKLIFTTHINGTURN_H
#define DYFORKLIFTTHINGTURN_H

#include "../agvtasknodedothing.h"

class DyForkliftThingTurn : public AgvTaskNodeDoThing
{
public:
    DyForkliftThingTurn(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 1 };

    int type(){return Type;}


    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("FORKLIFT TURN");
    }



private:
    int waitMs;
    float speed;
    float angle;
    bool bresult;//执行结果
};

#endif // DYFORKLIFTTHINGTURN_H
