#ifndef DYFORKLIFTTHINGCHARGE_H
#define DYFORKLIFTTHINGCHARGE_H

#include "../agvtasknodedothing.h"
#include "charge/chargemachine.h"

class DyForkliftThingCharge : public AgvTaskNodeDoThing
{
public:
    DyForkliftThingCharge(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 3 };

    int type(){return Type;}


    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("FORKLIFT CHARGE");
    }

private:
    std::string ip;
    int port;
    int charge_id;
    chargemachine cm;
    bool bresult;//执行结果

};

#endif // DYFORKLIFTTHINGCHARGE_H
