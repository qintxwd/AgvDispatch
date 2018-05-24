#ifndef QUNCHUANGNODETHINGGET_H
#define QUNCHUANGNODETHINGGET_H

#include "../agvtasknodedothing.h"

class QunChuangNodeThingGet : public AgvTaskNodeDoThing
{
public:
    QunChuangNodeThingGet(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 1 };

    int type(){return Type;}

    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("Qunchuang Agv GET_GOOD");
    }

private:
    bool bresult;//执行结果
};

#endif // QUNCHUANGNODETHINGGET_H
