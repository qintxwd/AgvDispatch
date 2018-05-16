#include "jacktaskmaker.h"
#include "../taskmanager.h"
#include "../mapmanager.h"
#include "../agvmanager.h"
#include "jackingagvthingturn.h"
#include "jackingagv.h"
#include "../userlogmanager.h"

JackTaskMaker::JackTaskMaker()
{

}

void JackTaskMaker::makeTask(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
    //TODO:创建任务//TODO:回头再改
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    UserLogManager::getInstance()->push(conn->getUserName()+" make one task");


    //生成任务



//    std::vector<std::string> all = splite(str," ");
//    //all助成部分:
//    //[agvid] [优先级] [do] [where] [do] [where]

//    //例如一个任务是指定 AGV 到A点取货，放到B点
//    //[0] [2] [get_good] [aId] [put_good] [bId]

//    int agvId = stringToInt( all[0]);
//    AgvPtr agv = AgvManager::getInstance()->getAgvById(agvId);
//    int priority = stringToInt(all[1]);

//    if(all.size()<4){
//        //参数不够
//        response.return_head.result = RETURN_MSG_RESULT_FAIL;
//        //TODO//response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
//    }else{
//        //产生一个任务
//        std::vector<AgvTaskNodePtr> nodes;
//        for(int i=2;i<all.size();){
//            if(all[i] == "get_good")
//            {
//                AgvTaskNodePtr node(new AgvTaskNode());
//                int stationId = stringToInt(all[i+1]);
//                AgvStationPtr station = MapManager::getInstance()->getStationById(stationId);
//                node->setStation(station);

//                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

//                //转身
//                std::vector<std::string> _params1;
//                _params1.push_back("turn 1 ");
//                _params1.push_back(intToString(90));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params1)));

//                //前进
//                std::vector<std::string> _params2;
//                _params2.push_back("forward ");
//                _params2.push_back(intToString(100));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params2)));

//                //取货

//                //后退
//                std::vector<std::string> _params4;
//                _params4.push_back("backward ");
//                _params4.push_back(intToString(100));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params4)));

//                //回身
//                std::vector<std::string> _params5;
//                _params5.push_back("turn 0 ");
//                _params5.push_back(intToString(90));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params5)));

//                node->setDoThings(getGoodDoThings);
//                nodes.push_back(node);
//            }
//            else  if(all[i] == "put_good"){
//                AgvTaskNodePtr node(new AgvTaskNode());
//                int stationId = stringToInt(all[i+1]);
//                AgvStationPtr station = MapManager::getInstance()->getStationById(stationId);
//                node->setStation(station);

//                std::vector<AgvTaskNodeDoThingPtr> getGoodDoThings;

//                //转身
//                std::vector<std::string> _params1;
//                _params1.push_back("turn 1 ");
//                _params1.push_back(intToString(90));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params1)));

//                //前进
//                std::vector<std::string> _params2;
//                _params2.push_back("forward ");
//                _params2.push_back(intToString(100));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params2)));

//                //放货货

//                //后退
//                std::vector<std::string> _params4;
//                _params4.push_back("backward ");
//                _params4.push_back(intToString(100));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params4)));

//                //回身
//                std::vector<std::string> _params5;
//                _params5.push_back("turn 0 ");
//                _params5.push_back(intToString(90));
//                getGoodDoThings.push_back(AgvTaskNodeDoThingPtr(new JackingAgvThingTurn(_params5)));

//                node->setDoThings(getGoodDoThings);
//                nodes.push_back(node);
//            }else{
//                //其他参数，无法识别，返回错误
//                //TODO:...
//            }

//            i+=2;
//        }

//        AgvTaskPtr task(new AgvTask());
//        task->setAgv(agv);
//        task->setTaskNode(nodes);
//        task->setPriority(priority);

//        //放入未分配的队列中
//        TaskManager::getInstance()->addTask(task);
//        int id = task->getId();

//        //返回产生任务的ID
//        response.head.body_length = sizeof(int32_t);
//        memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&id,sizeof(int32_t));
//    }

    conn->send(response);
}
