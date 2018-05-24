#include "qunchuangtaskmaker.h"

#include "../mapmap/mapmanager.h"
#include "../agvtask.h"
#include "qunchuangnodethingget.h"
#include "qunchuangnodetingput.h"
#include "../taskmanager.h"

QunChuangTaskMaker::QunChuangTaskMaker():
    tcsConnection(nullptr)
{

}

QunChuangTaskMaker::~QunChuangTaskMaker()
{
    if(tcsConnection!=nullptr){
        delete tcsConnection;
        tcsConnection = nullptr;
    }
}

void QunChuangTaskMaker::init()
{
    tcsConnection = new QunChuangTcsConnection("127.0.0.1",8888);
    tcsConnection->init();
}

void QunChuangTaskMaker::makeTask(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
    //群创不由客户端创建任务
    return ;
}

//创建一个取货+送货任务
void QunChuangTaskMaker::makeTask(std::string from ,std::string to,int dispatch_id,int ceid)
{
    auto fromSpirit = MapManager::getInstance()->getMapSpiritByName(from);
    auto toSpirit = MapManager::getInstance()->getMapSpiritByName(to);
    if(fromSpirit==nullptr || fromSpirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return ;
    if(toSpirit==nullptr || toSpirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return ;

    AgvTaskPtr task(new AgvTask());

    //两个参数
    task->setExtraParam("dispatch_id",intToString(dispatch_id));
    task->setExtraParam("ceid",intToString(ceid));

    //取货node
    AgvTaskNodePtr getNode(new AgvTaskNode());
    getNode->setStation(fromSpirit->getId());
    AgvTaskNodeDoThingPtr getThing(new QunChuangNodeThingGet(std::vector<std::string>()));
    getNode->push_backDoThing(getThing);

    //放货node
    AgvTaskNodePtr putNode(new AgvTaskNode());
    putNode->setStation(toSpirit->getId());
    AgvTaskNodeDoThingPtr putThing(new QunChuangNodeTingPut(std::vector<std::string>()));
    putNode->push_backDoThing(putThing);

    task->push_backNode(getNode);
    task->push_backNode(putNode);


    task->setProduceTime(getTimeStrNow());
    TaskManager::getInstance()->addTask(task);
}
