#include "qunchuangnodetingput.h"
#include "../agvImpl/ros/agv/rosAgv.h"

QunChuangNodeTingPut::QunChuangNodeTingPut(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{

}

void QunChuangNodeTingPut::beforeDoing(AgvPtr agv)
{
    //DONOTHING
}

void QunChuangNodeTingPut::doing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr rAgv((rosAgv *)agv.get());
    bresult = false;
    bool sendResult /*= rAgv->put()*/;
    bresult = true;
}


void QunChuangNodeTingPut::afterDoing(AgvPtr agv)
{
   //DONOTHING
}

bool QunChuangNodeTingPut::result(AgvPtr agv)
{
    return bresult;
}
