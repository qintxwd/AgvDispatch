#include "dytaskmaker.h"
#include "../taskmanager.h"
#include "../mapmap/mapmanager.h"
#include "../agvmanager.h"
#include "dyforkliftthingturn.h"
#include "dyforkliftthingfork.h"
#include "dyforkliftupdwms.h"
#include "dyforklift.h"
#include "../userlogmanager.h"
#include "../agvtask.h"
#include "qyhtcpclient.h"

DyTaskMaker::DyTaskMaker(std::string _ip, int _port):
    m_ip(_ip),
    m_port(_port),
    m_connectState(false)
{

}

void DyTaskMaker::init()
{
    //接收wms发过来的任务
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&DyTaskMaker::onRead, this, std::placeholders::_1, std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&DyTaskMaker::onConnect, this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&DyTaskMaker::onDisconnect, this);

    m_wms_tcpClient = new QyhTcpClient(m_ip, m_port, onread, onconnect, ondisconnect);

    combined_logger->info(typeid(TaskMaker::getInstance()).name());
}


void DyTaskMaker::onRead(const char *data, int len)
{
    receiveTask(data);
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


void DyTaskMaker::makeTask(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
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

            if (doWhat == 0) {

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
            }else if (doWhat == 1) {

                task_describe.append("[↓] ");

                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");
                doThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingFork(_paramsfork)));
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

    //UserLogManager::getInstance()->push(conn->getUserName()+" make one task");

    //    std::vector<std::string> all = split("0 2 get_good 6 put_good 23"," ");
    /*    std::vector<std::string> all = split("0 2 get_good 5 put_good 8"," ");

    //std::vector<std::string> all = split("0 2 get_good 6 put_good 110"," ");
    //    std::vector<std::string> all = split("0 2 get_good 4 put_good 3"," ");


    //        std::vector<std::string> all = splite(str," ");
    //all助成部分:
    //[agvid] [优先级] [do] [where] [do] [where]

    //例如一个任务是指定 AGV 到A点取货，放到B点
    //[0] [2] [get_good] [aId] [put_good] [bId]

    int agvId = stringToInt( all[0]);
    // AgvPtr agv = AgvManager::getInstance()->getAgvById(agvId);
    int priority = stringToInt(all[1]);

    if(all.size()<4){
        //参数不够
        response["result"] = RETURN_MSG_RESULT_FAIL;
        //TODO//response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
    }else{
        //产生一个任务
        std::vector<AgvTaskNodePtr> nodes;
        for(int i=2;i<all.size();){
            if(all[i] == "get_good")
            {
                AgvTaskNodePtr node(new AgvTaskNode());
                int stationId = stringToInt(all[i+1]);
                node->setStation(stationId);

                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

                //前进到目标点
                /*std::vector<std::string> _params1;
                _params1.push_back("move ");
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingMove(_params1)));
*/
    //旋转
    /*               std::vector<std::string> _params2;
                _params2.push_back("turn ");
                _params2.push_back(intToString(90));
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params2)));
*/
    //取货

    //后退
    /*std::vector<std::string> _params4;
                    _params4.push_back("backward ");
                    _params4.push_back(intToString(100));
                    getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params4)));

                    //回身
                    std::vector<std::string> _params5;
                    _params5.push_back("turn 0 ");
                    _params5.push_back(intToString(90));
                    getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params5)));
*/
    /*                node->setDoThings(getGoodDoThings);
                nodes.push_back(node);
            }
            else  if(all[i] == "put_good"){
                AgvTaskNodePtr node(new AgvTaskNode());
                int stationId = stringToInt(all[i+1]);
                node->setStation(stationId);

                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

                //转身
                /*  std::vector<std::string> _params1;
                    _params1.push_back("turn 1 ");
                    _params1.push_back(intToString(90));
                    getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params1)));
*/
    //前进
    /* std::vector<std::string> _params2;
                _params2.push_back("forward ");
                _params2.push_back(intToString(100));
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params2)));
*/
    //放货货

    //后退
    /*std::vector<std::string> _params4;
                    _params4.push_back("backward ");
                    _params4.push_back(intToString(100));
                    getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params4)));

                    //回身
                    std::vector<std::string> _params5;
                    _params5.push_back("turn 0 ");
                    _params5.push_back(intToString(90));
                    getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params5)));
*/
    /*                node->setDoThings(getGoodDoThings);
                nodes.push_back(node);
            }else{
                //其他参数，无法识别，返回错误
                //TODO:...
            }

            i+=2;
        }

        AgvTaskPtr task(new AgvTask());
        task->setAgv(agvId);
        task->setTaskNodes(nodes);
        task->setPriority(priority);

        //放入未分配的队列中
        TaskManager::getInstance()->addTask(task);
        int id = task->getId();

        //返回产生任务的ID
        //response.head.body_length = sizeof(int32_t);
        //memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&id,sizeof(int32_t));
    }

    //conn->send(response);*/
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

                //旋转
                //                std::vector<std::string> _params2;
                //                _params2.push_back("50");
                //                _params2.push_back("-10");
                //                _params2.push_back(intToString(90));
                //                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params2)));

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
                DyForkliftUpdWMS* test= new DyForkliftUpdWMS(_paramswms);
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(test));


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
                /*  std::vector<std::string> _params2;
                _params2.push_back("forward ");
                _params2.push_back(intToString(100));
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new DyForkliftThingTurn(_params2)));
*/

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

                DyForkliftUpdWMS* test= new DyForkliftUpdWMS(_paramswms);
                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(test));

                //放货
                node->setDoThings(getGoodDoThings);
                nodes.push_back(node);
                i+=5;
            }else{
                //其他参数，无法识别，返回错误
                //TODO:...
            }


        }

        AgvTaskPtr task(new AgvTask());
        task->setAgv(agvId);
        task->setTaskNodes(nodes);
        task->setPriority(priority);
        task->setExtraParam("runTimes", "1");
        task->setProduceTime(getTimeStrNow());
        task->setDescribe(task_describe);
        //放入未分配的队列中
        TaskManager::getInstance()->addTask(task);
        int id = task->getId();

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

//void makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info)
//{

//}
