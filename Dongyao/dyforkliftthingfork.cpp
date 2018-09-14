#include "dyforkliftthingfork.h"
#include <cassert>
#include "../common.h"
#include "dyforklift.h"


DyForkliftThingFork::DyForkliftThingFork(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params),
    forkParams(0)
{
    if(params.size()>=1){
        forkParams = stringToInt(params[0]);
    }
}

void DyForkliftThingFork::beforeDoing(AgvPtr agv)
{

}

void DyForkliftThingFork::doing(AgvPtr agv)
{
    if(agv->type() != DyForklift::Type){
        //other agv,can not excute dy forklift fork
        bresult = true;
        return ;
    }
//    combined_logger->debug("agv->type = {0},dyforklifttype = {1}",agv->type(),DyForklift::Type);

    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);

    bool sendResult = forklift->fork(forkParams);

    combined_logger->info("dothings-fork={0}",forkParams);

    do
    {
        sleep(1);
    }while(!forklift->isFinish(FORKLIFT_FORK));

    combined_logger->info("dothings-fork={0} end",forkParams);

    bresult = true;

}


void DyForkliftThingFork::afterDoing(AgvPtr agv)
{
}

bool DyForkliftThingFork::result(AgvPtr agv)
{
    //    Q_UNUSED(agv)
    return bresult;
}
