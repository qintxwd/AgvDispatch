#include "dytaskmaker.h"
#include "../taskmanager.h"
#include "../mapmap/mapmanager.h"
#include "../agvmanager.h"
#include "dyforkliftthingfork.h"
#include "dyforkliftthingcharge.h"
#include "dyforkliftupdwms.h"
#include "dyforklift.h"
#include "../userlogmanager.h"
#include "../agvtask.h"
#include "../network/tcpclient.h"

DyTaskMaker::DyTaskMaker(std::string _ip, int _port):
    m_ip(_ip),
    m_port(_port),
    m_connectState(false)
{

}

void DyTaskMaker::init()
{
    //接收wms发过来的任务
    TcpClient::ClientReadCallback onread = std::bind(&DyTaskMaker::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::ClientConnectCallback onconnect = std::bind(&DyTaskMaker::onConnect, this);
    TcpClient::ClientDisconnectCallback ondisconnect = std::bind(&DyTaskMaker::onDisconnect, this);

    m_wms_tcpClient = new TcpClient(m_ip, m_port, onread, onconnect, ondisconnect);
    m_wms_tcpClient->start();
    //combined_logger->info(typeid(TaskMaker::getInstance()).name());
}

int DyTaskMaker::msgProcess()
{
    int StartIndex,EndIndex;
    std::string FrameData;
    int start_byteFlag = buffer.find('*');
    int end_byteFlag = buffer.find('#');
    if((start_byteFlag ==-1)&&(end_byteFlag ==-1))
    {
        buffer.clear();
        return 5;
    }
    else if((start_byteFlag == -1)&&(end_byteFlag != -1))
    {
        buffer.clear();
        return 6;
    }
    else if((start_byteFlag != -1)&&(end_byteFlag == -1))
    {
        buffer.removeFront(start_byteFlag);
        return 7;
    }
    StartIndex = buffer.find('*')+1;
    EndIndex = buffer.find('#');
    if(EndIndex>StartIndex)
    {
        FrameData = buffer.substr(StartIndex,EndIndex-StartIndex);
        if(FrameData.find('*') != std::string::npos)
            FrameData = FrameData.substr(FrameData.find_last_of('*')+1);
        buffer.removeFront(buffer.find('#')+1);
    }
    else
    {
        //remove unuse message
        buffer.removeFront(buffer.find('*'));
        return 2;
    }

    unsigned int FrameLength = std::stoi(FrameData.substr(0,4));
    if(FrameLength == FrameData.length())
    {
        receiveTask(FrameData.substr(4));
    }
}

void DyTaskMaker::onRead(const char *data, int len)
{
    buffer.append(data, len);
    msgProcess();
}

void DyTaskMaker::onConnect()
{
    //TODO
    m_connectState = true;
    combined_logger->info("wms_connected. ip:{0} port:{1}", m_ip, m_port);
}

void DyTaskMaker::onDisconnect()
{
    //TODO
    m_connectState = false;
    combined_logger->info("wms_disconnected. ip:{0} port:{1}", m_ip, m_port);
}


void DyTaskMaker::makeTask(SessionPtr conn, const Json::Value &request)
{
    AgvTaskPtr task(new AgvTask());

    //1.指定车辆
    int agvId = request["agv"].asInt();
    task->setAgv(agvId);

    //2.优先级
    int priority = request["priority"].asInt();
    task->setPriority(priority);

    //3.执行次数
    if (!request["runTimes"].isNull())
    {
        int runTimes = request["runTimes"].asInt();
        task->setRunTimes(runTimes);
    }

    //3.额外的参数
    if (!request["extra_params"].isNull()) {
        Json::Value extra_params = request["extra_params"];
        Json::Value::Members mem = extra_params.getMemberNames();
        for (auto iter = mem.begin(); iter != mem.end(); iter++)
        {
            task->setExtraParam(*iter, extra_params[*iter].asString());
        }
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
            std::vector<AgvTaskNodeDoThingPtr> doThings;

            if (doWhat == TASK_PICK) {

                task_describe.append("[↑] ");
                //liftup
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("11");
                doThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingFork(_paramsfork)));


                //update wms
                /*  std::vector<std::string> _paramswms;
                _paramswms.push_back(all[i+2]);
                _paramswms.push_back(all[i+3]);
                _paramswms.push_back("0");
                _paramswms.push_back(all[i+4]);
                DyForkliftUpdWMS* test= new DyForkliftUpdWMS(_paramswms);
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(test));*/

                node_node->setTaskType(TASK_PICK);
                node_node->setDoThings(doThings);
            }else if (doWhat == TASK_PUT) {

                task_describe.append("[↓] ");

                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");
                doThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingFork(_paramsfork)));
                node_node->setTaskType(TASK_PUT);
                node_node->setDoThings(doThings);

            }else if (doWhat == TASK_CHARGE) {
                task_describe.append("[+] ");

                //charge
                std::vector<std::string> _paramscharge;
                MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
                if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;
                MapPoint *point = static_cast<MapPoint *>(spirit);
                _paramscharge.push_back(point->getLineId());  //充电桩id
                _paramscharge.push_back(point->getIp());
                _paramscharge.push_back(intToString(point->getPort()));

                doThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingCharge(_paramscharge)));
                node_node->setTaskType(TASK_CHARGE);
                node_node->setDoThings(doThings);
            }
            else if(doWhat == TASK_MOVE)
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

void DyTaskMaker::receiveTask(std::string str_task)
{
    combined_logger->info("receiveTask:{0}", str_task);

    std::vector<std::string> all = split(str_task);
    //all助成部分:
    //[agvid] [优先级] [do] [where] [do] [where]

    //例如一个任务是指定 AGV 到A点取货，放到B点
    //[0] [2] [get_good] [aId] [put_good] [bId]

    if(all.size()<4){
        combined_logger->warn("dytaskmaker recv task msg format error");
        //TODO
        //tell wms task make error to roll back database changes!
        return;
    }else{
        int agvId = stringToInt( all[0]);
        // AgvPtr agv = AgvManager::getInstance()->getAgvById(agvId);
        int priority = stringToInt(all[1]);
        std::string task_describe;

        //产生一个任务
        std::vector<AgvTaskNodePtr> nodes;
        for(int i=2;i<all.size();){
            if(all[i] == "pick")
            {
                AgvTaskNodePtr node(new AgvTaskNode());
                int stationId = stringToInt(all[i+1]);
                node->setStation(stationId);

                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

                //liftup
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("11");
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingFork(_paramsfork)));


                //update wms
                std::vector<std::string> _paramswms;
                _paramswms.push_back(all[i+2]);
                _paramswms.push_back(all[i+3]);
                _paramswms.push_back("0");
                _paramswms.push_back(all[i+4]);
                task_describe.append(all[i+2]).append("[").append(all[i+3]).append("]↑ ");
                DyForkliftUpdWMS* wms_task= new DyForkliftUpdWMS(_paramswms);
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(wms_task));


                //取货
                node->setDoThings(getGoodDoThings);
                nodes.push_back(node);
                i+=5;
            }
            else  if(all[i] == "put"){
                AgvTaskNodePtr node(new AgvTaskNode());
                int stationId = stringToInt(all[i+1]);
                node->setStation(stationId);

                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

                //前进
                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingFork(_paramsfork)));


                //update wms
                std::vector<std::string> _paramswms;
                _paramswms.push_back(all[i+2]);
                _paramswms.push_back(all[i+3]);
                _paramswms.push_back("1");
                _paramswms.push_back(all[i+4]);
                task_describe.append(all[i+2]).append("[").append(all[i+3]).append("]↓");

                DyForkliftUpdWMS* wms_task= new DyForkliftUpdWMS(_paramswms);
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(wms_task));

                //放货
                node->setDoThings(getGoodDoThings);
                nodes.push_back(node);
                i+=5;
            }else  if(all[i] == "move"){
                AgvTaskNodePtr node(new AgvTaskNode());
                int stationId = stringToInt(all[i+1]);
                node->setStation(stationId);
                nodes.push_back(node);
                i+=2;
            }
            else{
                //其他参数，无法识别，返回错误
                //TODO:...
                combined_logger->warn("dytaskmaker recv task msg format error");
                //TODO
                //tell wms task make error to roll back database changes!
                return ;
            }
        }

        AgvTaskPtr task(new AgvTask());
        task->setAgv(agvId);
        task->setTaskNodes(nodes);
        task->setPriority(priority);
        task->setRunTimes(1);
        task->setProduceTime(getTimeStrNow());
        task->setDescribe(task_describe);
        //放入未分配的队列中
        TaskManager::getInstance()->addTask(task);
        int id = task->getId();
        //tell wms task make success and the task id
        //TODO:
    }
}

void DyTaskMaker::finishTask(std::string store_no, std::string storage_no, int type, std::string key_part_no, int agv_id)
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

