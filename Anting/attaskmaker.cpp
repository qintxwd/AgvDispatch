#include "attaskmaker.h"
#include "../taskmanager.h"
#include "../mapmap/mapmanager.h"
#include "../agvmanager.h"
#include "atforkliftthingfork.h"
#include "atforklift.h"
#include "../userlogmanager.h"
#include "../agvtask.h"
#include "../network/tcpclient.h"

AtTaskMaker::AtTaskMaker(std::string _ip, int _port):
    m_ip(_ip),
    m_port(_port),
    m_connectState(false)
{

}

void AtTaskMaker::init()
{
    //接收wms发过来的任务
    TcpClient::ClientReadCallback onread = std::bind(&AtTaskMaker::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::ClientConnectCallback onconnect = std::bind(&AtTaskMaker::onConnect, this);
    TcpClient::ClientDisconnectCallback ondisconnect = std::bind(&AtTaskMaker::onDisconnect, this);

    m_wms_tcpClient = new TcpClient(m_ip, m_port, onread, onconnect, ondisconnect);

    combined_logger->info(typeid(TaskMaker::getInstance()).name());
}


void AtTaskMaker::onRead(const char *data, int len)
{
    //receiveTask(data);
}

void AtTaskMaker::onConnect()
{
    //TODO
    m_connectState = true;
    combined_logger->info("wms_connected. ip:{0} port:{1}", m_ip, m_port);
}

void AtTaskMaker::onDisconnect()
{
    //TODO
    m_connectState = false;
    combined_logger->info("wms_disconnected. ip:{0} port:{1}", m_ip, m_port);
}


void AtTaskMaker::makeTask(SessionPtr conn, const Json::Value &request)
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
        for (auto iter = mem.begin(); iter != mem.end(); iter++)
        {
            task->setExtraParam(*iter, extra_params[*iter].asString());
        }
    }
    else
    {
        task->setExtraParam("runTimes", "1");
    }

    std::string task_describe;
    //4.节点
    if (!request["nodes"].isNull()) {
        Json::Value nodes = request["nodes"];
        for (int i = 0; i < nodes.size(); ++i) {
            Json::Value one_node = nodes[i];
            int station = one_node["station"].asInt();
            int doWhat = one_node["dowhat"].asInt();
            std::string node_params_str = one_node["params"].asString();
            if(!node_params_str.length())
            {
                node_params_str = "20;20";
            }
            std::vector<std::string> node_params = split(node_params_str, ";");
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;
            MapPoint *point = static_cast<MapPoint *>(spirit);

            task_describe.append(point->getName());
            //根据客户端的代码，
            //dowhat列表为
            // 0 --> pick
            // 1 --> put
            // 2 --> charge
            AgvTaskNodePtr node_node(new AgvTaskNode());
            node_node->setStation(station);
            node_node->setParams(node_params_str);
            std::vector<AgvTaskNodeDoThingPtr> doThings;

            if (doWhat == 0) {

                task_describe.append("[↑] ");
                //liftup
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("11");
                //                _paramsfork.push_back(task->getExtraParam("moveHeight"));
                //                _paramsfork.push_back(task->getExtraParam("finalHeight"));
                if(node_params.size()>=2)
                {
                    _paramsfork.push_back(node_params[0]);
                    _paramsfork.push_back(node_params[1]);
                }

                //                task->setExtraParam("moveHeight", "1000");
                //                task->setExtraParam("finalHeight", "1100");

                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));

                node_node->setTaskType(TASK_PICK);
                node_node->setDoThings(doThings);
            }else if (doWhat == 1) {

                task_describe.append("[↓] ");

                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");

                if(node_params.size() >= 2)
                {
                    _paramsfork.push_back(node_params[0]);
                    _paramsfork.push_back(node_params[1]);
                }
                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));
                node_node->setTaskType(TASK_PUT);
                node_node->setDoThings(doThings);

            }else if (doWhat == 2) {
                //                AgvTaskNodeDoThingPtr chargeThing(new QingdaoNodeTingCharge(node_params));
                //                node_node->push_backDoThing(chargeThing);
            }
            else if(doWhat == 3)
            {
                task_describe.append("[--] ");

                //DONOTHING,JUST MOVE
                node_node->setTaskType(TASK_MOVE);
            }
            task->push_backNode(node_node);
        }
    }

    //5.产生时间
    task->setProduceTime(getTimeStrNow());
    task->setDescribe(task_describe);

    combined_logger->info(" getInstance()->addTask ");

    TaskManager::getInstance()->addTask(task);

    combined_logger->info("makeTask");

    //TODO:创建任务//TODO:回头再改
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
}



void AtTaskMaker::finishTask(std::string store_no, std::string storage_no, int type, std::string key_part_no, int agv_id)
{
    std::stringstream body;
    if(type == 1)
    {
        body<<"72"<<store_no<<"|"<<storage_no<<"|"<<agv_id<<"|"<<key_part_no;
    }
    else
    {
        body<<"73"<<store_no<<"|"<<storage_no<<"|"<<agv_id;
    }
    std::stringstream ss;
    ss<<"*";
    //时间戳
    ss.fill('0');
    ss.width(6);
    time_t   TimeStamp = clock()%1000000;
    ss<<TimeStamp;
    //长度
    ss.fill('0');
    ss.width(4);
    ss<<(body.str().length()+10);
    ss<<body.str();
    ss<<"#";

    combined_logger->info("sendToWMS:{0}", ss.str().c_str());


    m_wms_tcpClient->sendToServer(ss.str().c_str(),ss.str().length());

}
