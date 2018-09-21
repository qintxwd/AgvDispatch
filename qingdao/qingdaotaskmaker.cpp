#include "qingdaotaskmaker.h"
#include "../taskmanager.h"
#include "qingdaonodetingput.h"
#include "qingdaonodetingget.h"
#include "qingdaonodetingcharge.h"
#include "qingdaonodetingmove.h"

QingdaoTaskMaker::QingdaoTaskMaker()
{
}


QingdaoTaskMaker::~QingdaoTaskMaker()
{
}



void QingdaoTaskMaker::init()
{
    //nothing to do
    //start a thread make task! every 10 seconds
    //TODO:test dongyao ditu
    std::thread([&](){
        auto mapmanagerptr = MapManager::getInstance();
        auto taskmanagerptr = TaskManager::getInstance();

        std::vector<int> xianbianku;
        std::vector<int> chengpinku;
        std::vector<int> canxianku;
        std::vector<int> zhanbanku;
        std::vector<int> dengdaiku;

        int myarray1[] = { 249,251,258,267,268,274,275,276,277,89,280,356,439,440};
        xianbianku.insert (xianbianku.begin(), myarray1, myarray1+14);

        int myarray2[] = { 176,181 };
        chengpinku.insert (chengpinku.begin(), myarray2, myarray2+2);

        int myarray3[] = { 661,666,684,815,857 };
        canxianku.insert (canxianku.begin(), myarray3, myarray3+5);

        int myarray4[] = { 634,636 };
        zhanbanku.insert (zhanbanku.begin(), myarray4, myarray4+2);

        int myarray5[] = { 851,280 };
        dengdaiku.insert (dengdaiku.begin(), myarray5, myarray5+2);

        while(!g_quit){
            std::string sss;
            std::cin>>sss;
            if(sss == "start"){
                break;
            }
        }

        while(!g_quit)
        {
            sleep(10);
            if(taskmanagerptr->getCurrentTasks().size()<5){

                AgvTaskPtr task(new AgvTask());
                task->setAgv(0);
                task->setPriority(1);
                task->setProduceTime(getTimeStrNow());
                combined_logger->info(" getInstance()->addTask ");

                //4 : 1
                std::vector<std::string> node_params;
                int xx = getRandom(5);
                int nn;
                if(xx == 0){
                    //xianbian renwu
                    AgvTaskNodePtr node_node1(new AgvTaskNode());
                    node_node1->setStation(canxianku[getRandom(canxianku.size())]);
                    AgvTaskNodeDoThingPtr getThing1(new QingdaoNodeTingGet(node_params));
                    node_node1->push_backDoThing(getThing1);
                    task->push_backNode(node_node1);

                    AgvTaskNodePtr node_node2(new AgvTaskNode());
                    node_node2->setStation(xianbianku[getRandom(xianbianku.size())]);
                    AgvTaskNodeDoThingPtr getThing2(new QingdaoNodeTingPut(node_params));
                    node_node2->push_backDoThing(getThing2);
                    task->push_backNode(node_node2);

                    AgvTaskNodePtr node_node3(new AgvTaskNode());
                    node_node3->setStation(zhanbanku[getRandom(zhanbanku.size())]);
                    AgvTaskNodeDoThingPtr getThing3(new QingdaoNodeTingGet(node_params));
                    node_node3->push_backDoThing(getThing3);
                    task->push_backNode(node_node3);

                    AgvTaskNodePtr node_node4(new AgvTaskNode());
                    node_node4->setStation(node_node1->getStation());
                    AgvTaskNodeDoThingPtr getThing4(new QingdaoNodeTingPut(node_params));
                    node_node4->push_backDoThing(getThing4);
                    task->push_backNode(node_node4);

                    AgvTaskNodePtr node_node5(new AgvTaskNode());
                    node_node5->setStation(dengdaiku[getRandom(dengdaiku.size())]);
                    AgvTaskNodeDoThingPtr getThing5(new QingdaoNodeTingMove(node_params));
                    node_node5->push_backDoThing(getThing5);
                    task->push_backNode(node_node5);
                }else{
                    //chenpin renwu
                    AgvTaskNodePtr node_node1(new AgvTaskNode());
                    node_node1->setStation(xianbianku[getRandom(xianbianku.size())]);
                    AgvTaskNodeDoThingPtr getThing1(new QingdaoNodeTingGet(node_params));
                    node_node1->push_backDoThing(getThing1);
                    task->push_backNode(node_node1);

                    AgvTaskNodePtr node_node2(new AgvTaskNode());
                    node_node2->setStation(canxianku[getRandom(canxianku.size())]);
                    AgvTaskNodeDoThingPtr getThing2(new QingdaoNodeTingPut(node_params));
                    node_node2->push_backDoThing(getThing2);
                    task->push_backNode(node_node2);
                }

                TaskManager::getInstance()->addTask(task);
            }
        }
    }).detach();
}

void QingdaoTaskMaker::makeTask(SessionPtr conn, const Json::Value &request)
{
    AgvTaskPtr task(new AgvTask());

    //1.指定车辆
    int agvId = request["agv"].asInt();
    task->setAgv(agvId);

    //2.优先级
    int priority = request["priority"].asInt();
    task->setPriority(priority);

    //3.额外的参数
    if (!request["extra_params"].isNull()) {
        Json::Value extra_params = request["extra_params"];
        Json::Value::Members mem = extra_params.getMemberNames();
        for (auto iter = mem.begin(); iter != mem.end(); ++iter)
        {
            task->setExtraParam(*iter, extra_params[*iter].asString());
        }
    }

    //4.节点
    if (!request["nodes"].isNull()) {
        Json::Value nodes = request["nodes"];
        for (int i = 0; i < nodes.size(); ++i) {
            Json::Value one_node = nodes[i];
            int station = one_node["station"].asInt();
            int doWhat = one_node["dowhat"].asInt();
            std::string node_params_str = one_node["params"].asString();
            std::vector<std::string> node_params = split(node_params_str, ";");

            //根据客户端的代码，
            //dowhat列表为
            // 0 --> pick
            // 1 --> put
            // 2 --> charge
            // 3 --> move
            AgvTaskNodePtr node_node(new AgvTaskNode());
            node_node->setStation(station);

            if (doWhat == 0) {
                AgvTaskNodeDoThingPtr getThing(new QingdaoNodeTingGet(node_params));
                node_node->push_backDoThing(getThing);
            }else if (doWhat == 1) {
                AgvTaskNodeDoThingPtr putThing(new QingdaoNodeTingPut(node_params));
                node_node->push_backDoThing(putThing);
            }else if (doWhat == 2) {
                AgvTaskNodeDoThingPtr chargeThing(new QingdaoNodeTingCharge(node_params));
                node_node->push_backDoThing(chargeThing);
            }else if (doWhat == 2) {
                AgvTaskNodeDoThingPtr moveThing(new QingdaoNodeTingMove(node_params));
                node_node->push_backDoThing(moveThing);
            }
            task->push_backNode(node_node);
        }
    }

    //5.产生时间
    task->setProduceTime(getTimeStrNow());

    combined_logger->info(" getInstance()->addTask ");

    TaskManager::getInstance()->addTask(task);
}
