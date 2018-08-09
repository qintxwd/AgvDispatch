#include "atforkliftthingfork.h"
#include <cassert>
#include "../common.h"
#include "atforklift.h"


AtForkliftThingFork::AtForkliftThingFork(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params),
    forkParams(0)
{
    if(params.size()>=2){
        forkParams = stringToInt(params[0]);
        forkMoveHeight = stringToInt(params[1]);
        forkFinalHeight = stringToInt(params[2]);
    }
}

void AtForkliftThingFork::beforeDoing(AgvPtr agv)
{
    //DONOTHING
    //Q_UNUSED(agv)
}

void AtForkliftThingFork::doing(AgvPtr agv)
{
    AtForkliftPtr forklift = std::static_pointer_cast<AtForklift>(agv);

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

    if(forkParams)
    {
        //上扬
        forklift->forkAdjustAll(ATFORKLIFT_ADJUSTALL_UP, forkFinalHeight);
        combined_logger->info("dothings-forkadjustall={0}", ATFORKLIFT_ADJUSTALL_UP);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(ATFORKLIFT_FORK_ADJUSTALL));

        /*
        //上升至指定高度
        bool sendResult = forklift->fork(forkHeight);

        combined_logger->info("dothings-fork={0}",forkHeight);

        do
        {
            sleep(1);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_LIFT));

        //伸出
        forklift->forkAdjust(<<ATFORKLIFT_ADJUST_STRENCH);
        combined_logger->info("dothings-forkadjust={0}", <<ATFORKLIFT_ADJUST_STRENCH);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_ADJUST));

        //抬升10cm
        sendResult = forklift->fork(forkHeight+100);

        combined_logger->info("dothings-fork adjust height={0}",forkHeight);

        do
        {
            sleep(1);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_LIFT));

        //上扬
        forklift->forkAdjust(<<ATFORKLIFT_ADJUST_UP);
        combined_logger->info("dothings-forkadjust={0}", <<ATFORKLIFT_ADJUST_UP);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_ADJUST));
        */
    }
    else
    {
        //下扬
        forklift->forkAdjustAll(ATFORKLIFT_ADJUSTALL_DOWN, forkFinalHeight);
        combined_logger->info("dothings-forkadjustall={0}", ATFORKLIFT_ADJUSTALL_DOWN);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(ATFORKLIFT_FORK_ADJUSTALL));

        /*
        //下扬
        forklift->forkAdjust(<<ATFORKLIFT_ADJUST_DOWN);
        combined_logger->info("dothings-forkadjust={0}", <<ATFORKLIFT_ADJUST_DOWN);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_ADJUST));

        //下降至指定高度
        sendResult = forklift->fork(forkHeight);

        combined_logger->info("dothings-fork={0}",forkHeight);

        do
        {
            sleep(1);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_LIFT));

        //叉尺缩回
        forklift->forkAdjust(<<ATFORKLIFT_ADJUST_RETRACT);
        combined_logger->info("dothings-forkadjust={0}", <<ATFORKLIFT_ADJUST_RETRACT);

        do
        {
            sleep(2);
        }while(!forklift->isFinish(<<ATFORKLIFT_FORK_ADJUST));

*/
    }

    combined_logger->info("dothings-fork end");

}


void AtForkliftThingFork::afterDoing(AgvPtr agv)
{
    AtForkliftPtr forklift = std::static_pointer_cast<AtForklift>(agv);

    Pose4D pos = forklift->getPos();
    //update station
    forklift->setPosition(forklift->getNowStation(), forklift->nearestStation(pos.m_x*100, -pos.m_y*100, pos.m_theta*57.3, pos.m_floor), forklift->getNextStation());
    combined_logger->info("current station:{0}", forklift->getNowStation());
    bresult = true;
}

bool AtForkliftThingFork::result(AgvPtr agv)
{
//    Q_UNUSED(agv)
    return bresult;
}
