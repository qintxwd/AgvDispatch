#include "qunchuangtaskmaker.h"

#include "../mapmap/mapmanager.h"
#include "../agvtask.h"
#include "qunchuangnodethingget.h"
#include "qunchuangnodetingput.h"
#include "../taskmanager.h"
#include "../agvmanager.h"

QunChuangTaskMaker::QunChuangTaskMaker():
    tcsConnection(nullptr)
{

}

QunChuangTaskMaker::~QunChuangTaskMaker()
{
    // if(tcsConnection!=nullptr){
    //     delete tcsConnection;
    //     tcsConnection = nullptr;
    // }
}

void QunChuangTaskMaker::init()
{
    // tcsConnection = new QunChuangTcsConnection("192.168.2.113",2000);
    tcsConnection = QunChuangTcsConnection::Instance();
    tcsConnection->init();
}

void QunChuangTaskMaker::makeTask(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
    //群创不由客户端创建任务
    return ;
}

//创建一个取货+送货任务
void QunChuangTaskMaker::makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info)
{
    //combined_logger->info("makeTask 创建一个取货+送货任务, from: %s, to: %s", from, to);
    std::cout << "makeTask 创建一个取货+送货任务, dispatch_id:" + dispatch_id + " from: " + from + "  to: " + to <<"  all_floor_info: " << all_floor_info<< std::endl;

    auto fromSpirit = MapManager::getInstance()->getMapSpiritByName(from);
    auto toSpirit = MapManager::getInstance()->getMapSpiritByName(to);
    if(fromSpirit==nullptr || fromSpirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
    {
        combined_logger->info("makeTask  fromSpirit==nullptr ");
        //return ;
    }
    if(toSpirit==nullptr || toSpirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
    {
        combined_logger->info(" makeTask  toSpirit==nullptr ");
        //return ;
    }

    if(fromSpirit==nullptr && toSpirit==nullptr)
    {
        combined_logger->error(" makeTask  toSpirit==nullptr ");
        return ;
    }

    AgvTaskPtr task(new AgvTask());

    //4个参数
    task->setExtraParam("dispatch_id",dispatch_id);
    task->setExtraParam("ceid",intToString(ceid));
    task->setExtraParam("line_id",line_id);
    task->setExtraParam("agv_id",intToString(agv_id));

    //3层升降货架是否有料信息
    task->setExtraParam(ALL_FLOOR_INFO_KEY,intToString(all_floor_info));

    //AGV完成一个Task是否需要回到等待区
    task->setExtraParam("NEED_AGV_BACK_TO_WAITING_AREA_KEY","false");


    //取货node
    if(fromSpirit !=nullptr)
    {
        AgvTaskNodePtr getNode(new AgvTaskNode());
        getNode->setStation(fromSpirit->getId());
        AgvTaskNodeDoThingPtr getThing(new QunChuangNodeThingGet(std::vector<std::string>()));
        getThing->setStationId(fromSpirit->getId());
        getNode->push_backDoThing(getThing);
        task->push_backNode(getNode);
        combined_logger->info("makeTask, add  取货node");
    }

    //放货node
    if(toSpirit !=nullptr)
    {
        AgvTaskNodePtr putNode(new AgvTaskNode());
        putNode->setStation(toSpirit->getId());
        AgvTaskNodeDoThingPtr putThing(new QunChuangNodeTingPut(std::vector<std::string>()));
        putThing->setStationId(toSpirit->getId());

        putNode->push_backDoThing(putThing);
        task->push_backNode(putNode);

        //AGV完成一个Task是否需要回到等待区
        task->setExtraParam("NEED_AGV_BACK_TO_WAITING_AREA_KEY","true");
        combined_logger->info("makeTask add  放货node");
    }

    task->setProduceTime(getTimeStrNow());

    combined_logger->info(" getInstance()->addTask ");

    //任务指定了AGV ID
    AgvPtr agv = AgvManager::getInstance()->getAgvById(agv_id);
    if(agv != nullptr)
    {
        combined_logger->info(" 任务指定了AGV ID: "+intToString(agv_id));
        task->setAgv(agv_id);
        agv->onTaskStart(task);//通知AGV有新任务
    }
    else
    {
        combined_logger->info(" 任务未指定AGV");
    }

    TaskManager::getInstance()->addTask(task);
}
