#include "rosAgv.h"


bool rosAgv::send(const char *data, int len)
{
    if(bAgvConnected)
        return tcpClient->sendToServer(data, len);
    else
    {
        combined_logger->error("AGV {0} is not connected...", name);
        return false;
    }
}

void rosAgv::navCtrlStatusNotify(string waypoint_name, int nav_ctrl_status)
{
    if(nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED) //任务完成
    {
        combined_logger->info("task: " + waypoint_name + "完成, status: NAV_CTRL_STATUS_COMPLETED");
        if(waypoint_name == "init" || waypoint_name == "elevate_layer_1" || waypoint_name == "elevate_layer_1"  || waypoint_name == "init_home")
        {
            combined_logger->error("AGV {0} is reboot", name);
            setRebootState();
        }
        else
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
    msg["data"]="dbparam-update:" + map_name;
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

    if(currentTask == nullptr)
    {
        combined_logger->error("rosAgv, startTask, currentTask == nullptr");
        return false;
    }

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
         combined_logger->info("rosAgv, startTask, sendJsonToAGV end");

     }
     combined_logger->info("rosAgv, startTask, wait nav_ctrl_status");

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
        else
        {
            nav_ctrl_status_var.notify_all();
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

    //combined_logger->info("rosAgv, setAgvPath data: {0}", json.toStyledString());

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
    if(startTask("set_fixed"))
    {
        combined_logger->info("rosAgv set fixed path planner...");
        setAgvPath(agv_path);
        sleep(3);

        combined_logger->info("rosAgv report leave Station: {0}", nowStation);

        reportToTCSStation(nowStation, false);

        onLeaveStation(nowStation);

        combined_logger->info("sleep 1s,离开站点, {0}", nowStation);

        startTask(goal);

        combined_logger->info("到达站点, {0}", end->getId());
        onArriveStation(end->getId());

        startTask("set_navfn");

        nowStation = end->getId();
        lastStation = 0;

        reportToTCSStation(nowStation, true);

        combined_logger->info("rosAgv goStation, nowStation: {0}", nowStation);
        combined_logger->info("rosAgv goStation, lastStation: {0}", lastStation);
        combined_logger->info("rosAgv goStation, nextStation: {0}", nextStation);

    }
}


bool rosAgv::sendJsonToAGV(Json::Value json)
{
    bool result=false;
    int retry_count = 120;

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
                sleep(5);
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


bool rosAgv::isNeedTakeElevator(std::vector<int> lines)
{
    int endId = 0;
    combined_logger->info("rosAgv,  isNeedTakeElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        endId = path->getEnd();


        MapSpirit *pointSpirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *station = static_cast<MapPoint *>(pointSpirit);
        if (station == nullptr || station->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            combined_logger->error("rosAgv, station is null or not Map_Sprite_Type_Point!!!!!!!");
            continue;
        }

        if(station->getMapChange())
        {
            return true;
        }
    }
    return false;
}

std::vector<int> rosAgv::getPathToElevator(std::vector<int> lines)
{
    std::vector<int> paths;

    int endId = -1;
    int startId = -1;
    combined_logger->info("rosAgv,  getPathToElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;


        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        if(end_point->getMapChange())
        {
            return paths;
        }
        paths.push_back(line);

    }
    return paths;
}

std::vector<int> rosAgv::getPathLeaveElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("rosAgv,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if(addPath)
            paths.push_back(line);

        if(start_point->getMapChange() && end_point->getMapChange())
        {
            addPath = true;
        }
    }
    return paths;
}

std::vector<int> rosAgv::getPathInElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("rosAgv,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if(start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

std::vector<int> rosAgv::getPathEnterElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("rosAgv,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if(!start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

std::vector<int> rosAgv::getPathOutElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("rosAgv,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if(start_point->getMapChange() && !end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

void rosAgv::excutePath(std::vector<int> lines)
{
    std::vector<int> lines_to_elevator;//到达电梯口路径
    std::vector<int> lines_enter_elevator;//进入电梯路径
    std::vector<int> lines_take_elevator;//乘电梯到达楼层
    std::vector<int> lines_out_elevator;//出电梯路径
    std::vector<int> lines_leave_elevator;//离开电梯口去目标路径

    int agv_id = this->getId();

    combined_logger->info("rosAgv,  excutePath");



    if(!isNeedTakeElevator(lines))//不需要乘电梯
    {
        //告诉小车接下来要执行的路径
        goStation(lines, true);
    }//需要乘电梯
    else
    {
        lines_to_elevator = getPathToElevator(lines);//到达电梯口路径
        lines_enter_elevator = getPathEnterElevator(lines);//进入电梯路径
        lines_take_elevator = getPathInElevator(lines);//乘电梯到达楼层
        lines_out_elevator  = getPathOutElevator(lines);//出电梯路径
        lines_leave_elevator = getPathLeaveElevator(lines);//离开电梯去目标路径


        //暂时只有2号电梯, 后续需要根据电梯等待区设置呼叫某个ID电梯
        ElevatorPtr elevator = ElevatorManager::getInstance()->getElevatorById(2);

        if(elevator == nullptr)
        {
            combined_logger->error("rosAgv,  excutePath, no elevator!!!!!");
            return;
        }

        int from_floor = MapManager::getInstance()->getStationFloor(nowStation);

        int elevator_line = lines_take_elevator.at(0);

        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(elevator_line);
        MapPath *elevator_path = static_cast<MapPath *>(spirit);
        int endId = elevator_path->getEnd();

        int to_floor = MapManager::getInstance()->getStationFloor(endId);

        combined_logger->error("rosAgv,  excutePath, endId: {0}", endId);

        combined_logger->error("rosAgv,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);


        if(lines_to_elevator.size() > 0)
        {
            goStation(lines_to_elevator, true);//到达电梯口
        }

        //呼叫电梯到达楼层
        int result = -1;
        do
        {
            combined_logger->info("rosAgv,  excutePath, 呼叫电梯到达 {0} 楼, 30s超时继续呼叫电梯 ", from_floor);
            result = elevator->RequestTakeElevator(from_floor, 0, elevator->getId(), agv_id, 30);
        }
        while(result < 0);


        int try_times = 10;
        bool enter_ele = false;
        do
        {
            combined_logger->info("rosAgv,  excutePath, 发送乘梯应答 ");

            elevator->TakeEleAck(from_floor, to_floor, elevator->getId(), agv_id);
            std::cout << "等待电梯的进入指令...." << std::endl;
            combined_logger->info("rosAgv,  excutePath, 发送乘梯应答, 5s超时 ");

            enter_ele = elevator->ConfirmEleInfo(from_floor, to_floor, elevator->getId(), agv_id, 5);
            try_times--;
        }
        while(!enter_ele &&  try_times > 0);

        if( enter_ele == false )
        {
            combined_logger->error("rosAgv,  excutePath, ====no 等待电梯的进入指令....==================");
            return;
        }

        elevator->StartSendThread(lynx::elevator::TakeEleACK,from_floor, to_floor, elevator->getId(), agv_id);

        sleep(3); //等待电梯门完全打开

        if(lines_enter_elevator.size() > 0)
        {
            this->startTask("change_bz");
            combined_logger->error("rosAgv change local planner to bez...");
            goStation(lines_enter_elevator, true);//进入电梯
            this->startTask("change_dwa");
            combined_logger->error("rosAgv change local planner to dwa...");

        }

        elevator->StopSendThread();

        bool leave_elevator = false;

        do
        {
            elevator->AgvEnterConfirm(from_floor, to_floor, elevator->getId(), agv_id, 3);

            leave_elevator = elevator->AgvWaitArrive(from_floor, to_floor, elevator->getId(), agv_id, 30);
            if(leave_elevator)
            {
                combined_logger->info("rosAgv,  excutePath, ===电梯到达===");
            }
            else
            {
                combined_logger->error("rosAgv,  excutePath, ===电梯未到达===");
            }
        }
        while(!leave_elevator);

        if(leave_elevator)
        {
            string ros_dbparam_branch = INNOLUX_DB_BRANCH_SH1 + intToString(to_floor) + "F";

            combined_logger->debug("rosAgv,  excutePath, ros_dbparam_branch : {0}", ros_dbparam_branch);

            combined_logger->debug("rosAgv,  excutePath, *********************************");
            combined_logger->debug("rosAgv,  excutePath, ******start to change map********");
            combined_logger->debug("rosAgv,  excutePath, *********************************");

            changeMap(ros_dbparam_branch);

            //通知电梯AGV正在出电梯,离开过程中每5s发送一次【离开指令】，要求内呼等待机器人离开
            elevator->StartSendThread(lynx::elevator::LeftEleCMD,from_floor, to_floor, elevator->getId(), agv_id);

            sleep(5);//等待电梯门完全打开

            //AGV出电梯
            this->startTask("out_elevator");

            elevator->StopSendThread();//stop send 离开指令

            sleep(2);
            string agv_pose_task = INIT_AGV_POSE_OUT_ELEVATOR_NAME + intToString(to_floor) + "F";

            combined_logger->debug("rosAgv,  excutePath, init pose : {0}", agv_pose_task);

            this->startTask(agv_pose_task);

            if(lines_out_elevator.size() > 0)
            {
                int path_id = lines_out_elevator.at(lines_out_elevator.size()-1);
                MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(path_id);
                if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
                {
                    combined_logger->error("rosAgv,  excutePath, !MapSpirit::Map_Sprite_Type_Path");
                    return;
                }

                MapPath *path = static_cast<MapPath *>(spirit);

                this->nowStation = path->getEnd();
                combined_logger->debug("rosAgv,  excutePath, AGV出电梯, nowStation: {0}", nowStation);

                this->lastStation = 0;
                this->nextStation = 0;
            }
        }

        //通知电梯AGV完成出电梯
        bool leftEleStatus = elevator->AgvLeft(from_floor, to_floor, elevator->getId(), agv_id, 10);

        /*if(!leftEleStatus)//复位电梯状态, maybe elevator error
        {
            elevator->ResetElevatorState(elevator->getId());
        }*/

        if(lines_leave_elevator.size() > 0)
        {
            combined_logger->debug("rosAgv,  excutePath, 离开电梯口去目标");
            goStation(lines_leave_elevator, true);//离开电梯口去目标
        }
    }
}

