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
    //DONOTHING
//    Q_UNUSED(agv)

}

void DyForkliftThingFork::doing(AgvPtr agv)
{
    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);

    /*
    //TEST BEGIN
    bool sendResult = forklift->fork(!forkParams);

    combined_logger->info("dothings-fork={0}",forkParams);

    do
    {
        sleep(1);
    }while(!forklift->isFinish());
    //TEST END
    */
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
    //DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);

    //Pose4D pos = forklift->getPos();
    //update station
    //forklift->setPosition(forklift->getNowStation(), forklift->nearestStation(pos.m_x*100, -pos.m_y*100, pos.m_floor), forklift->getNextStation());
    //combined_logger->info("current station:{0}", forklift->getNowStation());
    //bresult = true;
}

bool DyForkliftThingFork::result(AgvPtr agv)
{
//    Q_UNUSED(agv)
    return bresult;
}
