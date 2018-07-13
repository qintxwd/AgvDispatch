#include "dyforkliftthingturn.h"
#include <cassert>
#include "../common.h"
#include "dyforklift.h"


DyForkliftThingTurn::DyForkliftThingTurn(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params),
    waitMs(0),
    speed(0),
    angle(0)
{
    if(params.size()>=3){
        waitMs = stringToInt(params[0]);
        speed = stof(params[1]);
        angle = stringToInt(params[2]);
    }
}

void DyForkliftThingTurn::beforeDoing(AgvPtr agv)
{
    //DONOTHING
//    Q_UNUSED(agv)
}

void DyForkliftThingTurn::doing(AgvPtr agv)
{
    //    assert(agv->type() == DyForklift::Type);
    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);
    //DyForkliftPtr jap = dynamic_pointer_cast<DyForklift*> agv;//((DyForklift *)agv.get());
    //    jap.reset((DyForklift *)agv.get());
    bresult = false;
    bool sendResult = forklift->turn(speed,angle);

    combined_logger->info("dothings-turn={0}",angle);

    do
    {
        bresult = forklift->waitTurnEnd(waitMs);
    }while(!forklift->isFinish(FORKLIFT_TURN));

    combined_logger->info("dothings-turn={0} end",angle);

}


void DyForkliftThingTurn::afterDoing(AgvPtr agv)
{
    //DONOTHING
//    Q_UNUSED(agv)

}

bool DyForkliftThingTurn::result(AgvPtr agv)
{
//    Q_UNUSED(agv)
    return bresult;
}
