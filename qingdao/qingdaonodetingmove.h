#ifndef QINGDAONODETINGMOVE_H
#define QINGDAONODETINGMOVE_H

#include "../agvtasknodedothing.h"

class QingdaoNodeTingMove:
        public AgvTaskNodeDoThing
{
public:
    QingdaoNodeTingMove(std::vector<std::string> _params);
    virtual ~QingdaoNodeTingMove();

    enum { Type = AgvTaskNodeDoThing::Type + 23 };

    int type() { return Type; }

    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe() {
        return std::string("Qingdao Agv GET_GOOD");
    }
private:
    bool bresult;
};

#endif // QINGDAONODETINGMOVE_H
