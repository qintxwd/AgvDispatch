#include "rosAgv.h"

void rosAgv::navCtrlStatusNotify(string waypoint_name, int nav_ctrl_status)
{
    if(nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED) //任务完成
    {
        combined_logger->info("task: " + waypoint_name + "完成, status: NAV_CTRL_STATUS_COMPLETED");
        nav_ctrl_status_var.notify_all();
    }
    else if(nav_ctrl_status == NAV_CTRL_STATUS_ERROR)//任务出错
    {
        combined_logger->info("task: " + waypoint_name + "出错, status: NAV_CTRL_STATUS_ERROR");
    }
    else if(nav_ctrl_status == NAV_CTRL_STATUS_CANCELLED)//任务取消
    {
        combined_logger->info("task: " + waypoint_name + "取消, status: NAV_CTRL_STATUS_CANCELLED");
    }
}

void rosAgv::parseJsondata(const char *data,int len)
{
    Json::Reader reader;
    Json::Value root;

    try{
        if (!reader.parse(data, root))
        {
          combined_logger->error("rosAgv, parse json error!!!");
          combined_logger->info("data : " + string(data));
          return;
        }
    }
    catch(exception e)
    {
        combined_logger->error("rosAgv parseJsondata, exception" );
    }

    if(root.isMember("op")){
        if(root["op"] == "service_response")
        {
            processServiceResponse(root);
        }
        else if(root["op"] == "call_service")
        {
            processServiceCall(root);
        }
        else if(root["op"] == "publish")
        {
            processPublishMsg(root);
        }

    }

}


void rosAgv::processServiceResponse(Json::Value response)
{
    try{
        bool result=response["result"].asBool();

        if(response.isMember("service")){
            string service_name = response["service"].asString();
            if(result)
            {
                if(service_name.find("nav_ctrl_service") != string::npos)
                {
                    bool success = response["values"]["success"].asBool();
                    if(success)
                        combined_logger->info("rosAgv,send task success");
                    else
                        combined_logger->error("rosAgv,send task fail");
                }
                else if(service_name.find("set_planner_path") != string::npos)
                {
                    bool feedback = response["values"]["feedback"].asInt();
                    if(feedback > 0)
                        combined_logger->info("rosAgv, set path 成功");
                    else
                        combined_logger->error("rosAgv, set path 失败");

                }
            }
            else
            {
                combined_logger->error("rosAgv, call_service: " + service_name + "error!");
            }
        }
    }
    catch(exception e)
    {

    }
}



void rosAgv::processServiceCall(Json::Value call_service)
{
    try
    {
        if(call_service.isMember("service")){
            string service_name = call_service["service"].asString();
            if(service_name.find("nav_ctrl_status_service") != string::npos)
            {
                combined_logger->info("this is a nav_ctrl_status_service");

                //获得 nav ctrl status 状态
                if(call_service.isMember("args")){
                    Json::Value args = call_service["args"];
                    m_nav_ctrl_status=args["status"].asInt();
                    string waypoint_name = args["waypoint_name"].asString();

                    navCtrlStatusNotify(waypoint_name, m_nav_ctrl_status);
                }

                //send service response
                Json::Value value;
                value["received"]=true;
                string id = call_service.isMember("id")? call_service["id"].asString():"";

                sendServiceResponse(service_name, &value, id);
            }
        }


    }
    catch(exception e)
    {
        combined_logger->error("rosAgv, processServiceCall exception...");
    }
}


void rosAgv::processPublishMsg(Json::Value json)
{
    if(json.isMember("topic"))
    {
        string topic=json["topic"].asString();
        if(topic == "/waypoint_user_sub")
        {
            if(json.isMember("msg"))
            {
                string status=json["msg"]["data"].asString();
                //get 3层升降货架 up_part_status or down_part_status
                if(status.find("part_status") != string::npos)
                {
                    if(status.find("0") == string::npos)
                    {
                        if(status.find("up_part_status") != string::npos)
                        {
                            if(status.find("up_part_status_1") != string::npos)
                            {
                                shelf_finished_status="up_part_status:"+status.substr(status.find(":")+1);
                                combined_logger->info("1层升降货架上料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                            else if(status.find("up_part_status_2") != string::npos)
                            {
                                shelf_finished_status=shelf_finished_status+","+status.substr(status.find(":")+1);
                                combined_logger->info("2层升降货架上料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                            else if(status.find("up_part_status_3") != string::npos)
                            {
                                shelf_finished_status=shelf_finished_status+","+status.substr(status.find(":")+1);
                                combined_logger->info("3层升降货架上料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                        }
                        else
                        {
                            if(status.find("down_part_status_1") != string::npos)
                            {
                                shelf_finished_status="down_part_status:"+status.substr(status.find(":")+1);
                                combined_logger->info("1层升降货架下料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                            else if(status.find("down_part_status_2") != string::npos)
                            {
                                shelf_finished_status=shelf_finished_status+","+status.substr(status.find(":")+1);
                                combined_logger->info("2层升降货架下料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                            else if(status.find("down_part_status_3") != string::npos)
                            {
                                shelf_finished_status=shelf_finished_status+","+status.substr(status.find(":")+1);
                                combined_logger->info("3层升降货架下料结束, shelf_finished_status:" + shelf_finished_status);
                            }
                        }
                    }
                }
            }
        }
    }
}


void rosAgv::sendServiceResponse(string service_name,Json::Value *value,string id)
{
    Json::Value response;
    response["op"]="service_response";
    response["service"]=service_name;
    response["result"]=true;
    if(value != nullptr)
        response["values"]=*value;
    if(id != "")
        response["id"]=id;


    if(!sendJsonToAGV(response))
        combined_logger->error("rosAgv, sendServiceResponse: " + string(service_name) + "error...");
}

void rosAgv::changeMap(string map_name)
{
    Json::Value msg;
    msg["data"]="dbparam-update:test2";
    publishTopic("/rosnodejs/cmd_string", msg);
}

void rosAgv::subTopic(const char * topic, const char * topic_type)
{
    Json::Value json;
    json["op"]="subscribe";
    json["topic"]=topic;
    json["type"]=topic_type;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, subTopic: " + string(topic) + "error...");
}

void rosAgv::unSubTopic(const char * topic)
{
    Json::Value json;
    json["op"]="unsubscribe";
    json["topic"]=topic;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, unSubTopic: " + string(topic) + "error...");
}

void rosAgv::advertiseTopic(const char * topic, const char * topic_type)
{
    Json::Value json;
    json["op"]="advertise";
    json["topic"]=topic;
    json["type"]=topic_type;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, advertiseTopic: " + string(topic) + "error...");
}

void rosAgv::advertiseService(const char * service_name, const char * msg_type)
{
    Json::Value json;
    json["op"]="advertise_service";
    json["service"]=service_name;
    json["type"]=msg_type;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, advertiseTopic: " + string(service_name) + "error...");
}

void rosAgv::publishTopic(const char * topic, Json::Value msg)
{
    Json::Value json;
    json["op"]="publish";
#if SIMULATOR
    json["topic"]=getName()+topic;
#else
    json["topic"]=topic;
#endif
    json["msg"]=msg;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, publishTopic: " + string(topic) + "error...");
}

bool rosAgv::startTask(std::string task_name)
{
    std::unique_lock <std::mutex> lock(nav_ctrl_status_mutex);

    combined_logger->info("rosAgv, startTask: " + task_name);

    m_nav_ctrl_status = NAV_CTRL_STATUS_IDLING;

     if(NAV_CTRL_USING_TOPIC)
     {
         Json::Value msg;
         msg["goal_name"]=task_name;
         msg["control"]=START;
         publishTopic("/nav_ctrl", msg);

     }
     else
     {
         Json::Value json;
         Json::Value nav_ctrl_msg;
         Json::Value nav_ctrl_srv;

         nav_ctrl_msg["goal_name"]=task_name;
         nav_ctrl_msg["control"]=START;

         nav_ctrl_srv["msg"]=nav_ctrl_msg;

         json["op"]="call_service";
#if SIMULATOR
         json["service"]=getName() + "/nav_ctrl_service";
#else
         json["service"]="/nav_ctrl_service";
#endif
         json["args"]=nav_ctrl_srv;

         combined_logger->info("rosAgv, startTask: sendJsonToAGV");

         if(!sendJsonToAGV(json))
         {
             combined_logger->error("rosAgv, start task: " +  task_name + " error...");
             return false;
         }
     }
     nav_ctrl_status_var.wait(lock);

     if(m_nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED)
     {
        combined_logger->info("rosAgv, startTask: " + task_name + " finished!");
        return true;
     }
     else
     {
         combined_logger->error("rosAgv, startTask: " + task_name + " unfinished!");
         return false;
     }
}

void rosAgv::cancelTask()
{
    if(NAV_CTRL_USING_TOPIC)
    {
        Json::Value msg;
        msg["goal_name"]="";
        msg["control"]=STOP;
#if SIMULATOR
        publishTopic((getName() + "/nav_ctrl").c_str(), msg);
#else
        publishTopic("/nav_ctrl", msg);
#endif
    }
    else
    {
        Json::Value json;
        Json::Value nav_ctrl_msg;
        Json::Value nav_ctrl_srv;

        nav_ctrl_msg["goal_name"]="";
        nav_ctrl_msg["control"]=STOP;

        nav_ctrl_srv["msg"]=nav_ctrl_msg;

        json["op"]="call_service";
#if SIMULATOR
        json["service"]=getName() + "/nav_ctrl_service";
#else
        json["service"]="/nav_ctrl_service";
#endif
        json["args"]=nav_ctrl_srv;

        if(!sendJsonToAGV(json))
        {
            combined_logger->error("rosAgv, cancelTask task error");
        }
    }
}


void rosAgv::setAgvPath(std::vector<Pose2D> path)
{
    Json::Value json;
    Json::Value msg;
    Json::Value header;
    Json::Value poses;

    header["frame_id"]="map";
    header["stamp"]=0;

    for(int i=0; i<path.size(); i++)
    {
        Pose2D p = path.at(i);
        poses[i]["x"]=p.x;
        poses[i]["y"]=p.y;
        poses[i]["theta"]=p.theta;
    }

    msg["header"]=header;
    msg["priority"]=0;
    msg["pathID"]=getId();
    msg["poses"]=poses;

    json["op"]="call_service";
#if SIMULATOR
    json["service"]=getName() + "/set_planner_path";
#else
    json["service"]="/set_planner_path";
#endif
    json["args"]=msg;

    if(!sendJsonToAGV(json))
        combined_logger->error("rosAgv, setAgvPath error...");

}

void rosAgv::goStation(std::vector<int> lines,  bool stop)
{
    MapPoint *start;
    MapPoint *end;

    std::vector<Pose2D> agv_path;
    string goal;

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int startId = path->getStart();
        int endId = path->getEnd();

        //获取站点：
        MapSpirit *spirit_start = MapManager::getInstance()->getMapSpiritById(startId);
        if (spirit_start == nullptr || spirit_start->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        MapSpirit *spirit_end = MapManager::getInstance()->getMapSpiritById(endId);
        if (spirit_end == nullptr || spirit_end->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        start = static_cast<MapPoint *>(spirit_start);
        start->getRealX();
        start->getName();
        start->getRealY();

        end = static_cast<MapPoint *>(spirit_end);
        end->getRealX();
        end->getName();
        end->getRealY();

        combined_logger->info("rosAgv goStation start: " + start->getName());
        combined_logger->info("rosAgv goStation end: " + end->getName());

        goal = end->getName();

        std::vector<Pose2D> path0 = LinePath::getLinePath_mm(start->getRealX(),start->getRealY(),end->getRealX(),end->getRealY());
        agv_path.insert(agv_path.end(),path0.begin(),path0.end());
        LinePath::writeToText("path_test", agv_path, false);
    }

    if(agv_path.size() <= 0)
    {
        combined_logger->error("rosAgv goStation no path...");
        return;
    }
    startTask("set_fixed");
    combined_logger->info("rosAgv set fixed path planner...");
    setAgvPath(agv_path);
    startTask(goal);
    startTask("set_navfn");
    combined_logger->info("rosAgv set navfn path planner...");
}

void rosAgv::excutePath(std::vector<int> lines)
{
    int endId = 0;

    combined_logger->info("rosAgv,  excutePath");

    //std::unique_lock <std::mutex> lock(nav_ctrl_status_mutex);

    stationMtx.lock();
    excutestations.clear();
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        endId = path->getEnd();
        excutestations.push_back(endId);
    }
    stationMtx.unlock();
    //告诉小车接下来要执行的路径
    goStation(lines, true);

    nowStation = endId;
}

bool rosAgv::sendJsonToAGV(Json::Value json)
{
    bool result=false;
    int retry_count = 10;

    std::string str_json = json.toStyledString();

    char *copy_temp = new char[str_json.size() + 1];

    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());

        while(retry_count > 0)
        {
            if(!send((const char *)copy_temp, json.toStyledString().size()))
            {
                combined_logger->error("rosAgv, sendJsonToAGV failed, send data error, retry_conut: {0}", retry_count);
                result=false;
                retry_count--;
                sleep(2);
            }
            else
            {
                result=true;
                break;
            }
        }

        delete[] copy_temp;
    }
    else
    {
        combined_logger->error("rosAgv, sendJsonToAGV error...");
        result=false;
    }

    return result;
}

bool rosAgv::secondaryLocalization(MapPoint* mapPoint)
{
    combined_logger->error("rosAgv, secondaryLocalization start...");

    //startTask("");

    combined_logger->error("rosAgv, secondaryLocalization end...");

    return true;
}

bool rosAgv::secondaryLocalization(string station)
{
    combined_logger->error("rosAgv, secondaryLocalization start...");

    startTask(station + "_second_location");

    combined_logger->error("rosAgv, secondaryLocalization end...");

    return true;
}



lynx::Msg rosAgv::getTcsMsg()
{
    if(currentTask != nullptr)
    {
        combined_logger->info("rosAgv,  getTcsMsg");

        string dispatch_id = currentTask->getExtraParam("dispatch_id");

        if(dispatch_id.length() > 0)
        {
            combined_logger->info("rosAgv, getTcsMsg, dispatch_id: " + dispatch_id);

            return QunChuangTcsConnection::Instance()->getTcsMsgByDispatchId(dispatch_id);

            /*std::string from    = msg_data["FROM"].string_value();
            std::string to      = msg_data["DESTINATION"].string_value();
            int ceid            = std::stoi(msg_data["CEID"].string_value());
            std::string dispatch_id  = msg_data["DISPATCH_ID"].string_value();

            combined_logger->info(" excutePath, from : " + from);
            combined_logger->info("                       ceid :{0} ", ceid);
            combined_logger->info("                       to : " + to);
            combined_logger->info("              dispatch_id : " + dispatch_id);
            */

        }
        else
            return nullptr;
    }
    else
        return nullptr;
}

