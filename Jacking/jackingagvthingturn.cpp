#include "jackingagvthingturn.h"
#include <cassert>
#include "common.h"
#include "jackingagv.h"


JackingAgvThingTurn::JackingAgvThingTurn(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params),
    waitMs(0),
    left(true),
    angle(0)
{
    if(params.size()>=3){
        waitMs = stringToInt(params[0]);
        left = stringToBool(params[1]);
        angle = stringToInt(params[2]);
    }
}

void JackingAgvThingTurn::beforeDoing(AgvPtr agv)
{
    //DONOTHING
}

void JackingAgvThingTurn::doing(AgvPtr agv)
{
    assert(agv->type() == JackingAgv::Type);
    JackingAgvPtr jap((JackingAgv *)agv.get());
//    jap.reset((JackingAgv *)agv.get());
    bresult = false;
    bool sendResult = jap->turn(left,angle);
    if(sendResult)return ;
    bresult = jap->waitTurnEnd(waitMs);
}


void JackingAgvThingTurn::afterDoing(AgvPtr agv)
{
   //DONOTHING
}

bool JackingAgvThingTurn::result(AgvPtr agv)
{
    return bresult;
}
