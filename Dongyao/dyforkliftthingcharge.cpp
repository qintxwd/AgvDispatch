#include "dyforkliftthingcharge.h"
#include <cassert>
#include "../common.h"
#include "dyforklift.h"

DyForkliftThingCharge::DyForkliftThingCharge(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{
    if(params.size()>=3){
        charge_id = stringToInt(params[0]);
        ip = params[1];
        port = stringToInt(params[2]);
        cm.setParams(charge_id, "charge_machine", ip, port,COMM_TIMEOUT);
    }
}

void DyForkliftThingCharge::beforeDoing(AgvPtr agv)
{
    bresult = false;
    //充电桩初始化
    if(cm.init())
    {
        cm.start();
    }
    else
    {
        combined_logger->error("init chargemachine error");
    }
}

void DyForkliftThingCharge::doing(AgvPtr agv)
{
    if(!cm.checkConnection())
        return;

    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);
    //叉尺抬升
    combined_logger->info("charge fork up start");
    bool sendResult = forklift->fork(FORKLIFT_UP);

    if(!sendResult)
    {
        combined_logger->info("send fork up error");
        return;
    }
    do
    {
        sleep(1);
    }while(!forklift->isFinish(FORKLIFT_FORK));

    combined_logger->info("charge fork up end");
    //发送给小车充电
    sendResult = forklift->charge(FORKLIFT_START_CHARGE);
    do
    {
        sleep(1);
    }while(!forklift->isFinish(FORKLIFT_CHARGE));


    combined_logger->info("dothings-charge start");

    //开始充电
    cm.chargeControl(charge_id, CHARGE_START);
    do
    {
        sleep(60);
        cm.chargeControl(charge_id, CHARGE_START);
        //等待充电完成
        //sleep(120);
    }while(CHARGE_FULL != cm.getStatus());
}


void DyForkliftThingCharge::afterDoing(AgvPtr agv)
{
    //通知小车退出充电
    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);

    bool sendResult = forklift->charge(FORKLIFT_END_CHARGE);
    do
    {
        sleep(1);
    }while(!forklift->isFinish(FORKLIFT_CHARGE));

    //通知充电桩停止充电
    //TODO 低温保护需退出
    cm.chargeControl(charge_id, CHARGE_STOP);
    do
    {
        sleep(1);
        cm.chargeControl(charge_id, CHARGE_STOP);
    }while(CHARGE_IDLE != cm.getStatus());

    //叉尺下降
    combined_logger->info("charge fork down start");
    sendResult = forklift->fork(FORKLIFT_DOWN);

    if(!sendResult)
    {
        combined_logger->info("send fork down error");
        return;
    }
    do
    {
        sleep(1);
    }while(!forklift->isFinish(FORKLIFT_FORK));

    combined_logger->info("charge fork down end");
    combined_logger->info("dothings-charge end");
    bresult = true;
}

bool DyForkliftThingCharge::result(AgvPtr agv)
{
    //    Q_UNUSED(agv)
    return bresult;
}
