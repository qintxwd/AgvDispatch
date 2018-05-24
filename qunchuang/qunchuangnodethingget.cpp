#include "qunchuangnodethingget.h"
#include "../agvImpl/ros/agv/rosAgv.h"

QunChuangNodeThingGet::QunChuangNodeThingGet(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{

}
void QunChuangNodeThingGet::beforeDoing(AgvPtr agv)
{
    //DONOTHING
}

void QunChuangNodeThingGet::doing(AgvPtr agv)
{
    assert(agv->type() == rosAgv::Type);
    rosAgvPtr rAgv((rosAgv *)agv.get());
//    jap.reset((JackingAgv *)agv.get());
    bresult = false;
    bool sendResult /*= rAgv->get()*/;
    bresult = true;
}


void QunChuangNodeThingGet::afterDoing(AgvPtr agv)
{
   //DONOTHING
}

bool QunChuangNodeThingGet::result(AgvPtr agv)
{
    return bresult;
}
