#include "rosAgv.h"
#include "mapmap/mappoint.h"
//#include <locale>
#include <condition_variable>


rosAgv::rosAgv(int id, std::string name, std::string ip, int port):
    Agv(id,name,ip,port)
{
    mChipmounter = nullptr;

    m_bInitlayer = false;
    m_agv_type = AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF;
}

void rosAgv::onConnect()
{
    combined_logger->info("rosAgv onConnect OK! name: "+getName());
#if SIMULATOR
    //subTopic((getName() + AGV_POSE_TOPIC_NAME).c_str(), AGV_POSE_TOPIC_TYPE);
    subTopic((getName() + "/rosnodejs/shell_feedback").c_str(), "std_msgs/String");
#else
    subTopic("/rosnodejs/shell_feedback", "std_msgs/String");
#endif
    if(NAV_CTRL_USING_TOPIC)
    {
#if SIMULATOR
        advertiseTopic((getName() + "/nav_ctrl").c_str(), "yocs_msgs/NavigationControl");
        advertiseTopic((getName() + "/rosnodejs/cmd_string").c_str(), "std_msgs/String");
        advertiseTopic((getName() + "/waypoint_user_pub").c_str(), "std_msgs/String");
#else
        advertiseTopic("/nav_ctrl", "yocs_msgs/NavigationControl");
        advertiseTopic("/rosnodejs/cmd_string", "std_msgs/String");
#endif
    }
    else //使用service接收AGV Navigation control status
    {
#if SIMULATOR
        advertiseService((getName() +"/nav_ctrl_status_service").c_str(),"scheduling_msgs/ReportNavigationControlStatus");
#else
        advertiseService("/nav_ctrl_status_service","scheduling_msgs/ReportNavigationControlStatus");
#endif
    }

    if(m_agv_type == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
    {
        InitShelfLayer();
    }
    //test();

    //changeMap("");

}

void rosAgv::onDisconnect()
{
}

void rosAgv::onRead(const char *data,int len)
{
    try{
        parseDataMtx.lock();
        combined_logger->info("111, rosAgv onRead, data : " + string(data));
        parseJsondata(data,len);
        parseDataMtx.unlock();
    }
    catch(exception e)
    {
        combined_logger->error("rosAgv onRead, exception" );
    }
}

void rosAgv::navCtrlStatusNotify(string waypoint_name, int nav_ctrl_status)
{
    //std::unique_lock <std::mutex> lock(nav_ctrl_status_mutex);

    if(nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED) //任务完成
    {
        combined_logger->info("task: " + waypoint_name + "完成, status: NAV_CTRL_STATUS_COMPLETED");
    }
    else if(nav_ctrl_status == NAV_CTRL_STATUS_ERROR)//任务出错
    {
        combined_logger->info("task: " + waypoint_name + "出错, status: NAV_CTRL_STATUS_ERROR");
    }
    else if(nav_ctrl_status == NAV_CTRL_STATUS_CANCELLED)//任务取消
    {
        combined_logger->info("task: " + waypoint_name + "取消, status: NAV_CTRL_STATUS_CANCELLED");
    }

    nav_ctrl_status_var.notify_all();
}

void rosAgv::parseJsondata(const char *data,int len)
{
    Json::Reader reader;
    Json::Value root;

    try{
        if (!reader.parse(data, root))
        {
          combined_logger->error("rosAgv, parse json error!!!");
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

    }
    else
    {

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
                        combined_logger->info("rosAgv, 发送任务成功");
                    else
                        combined_logger->error("rosAgv, 发送任务失败");
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
                    nav_ctrl_status=args["status"].asInt();
                    string waypoint_name = args["waypoint_name"].asString();

                    navCtrlStatusNotify(waypoint_name, nav_ctrl_status);
                }

                //send service response
                Json::Value value;
                value["received"]=true;
                string id = call_service.isMember("id")? call_service["id"].asString():"";

                //combined_logger->error("don't send Service Response to agv");

                combined_logger->info("send Service Response to agv");
                sendServiceResponse(service_name, &value, id);
            }
        }


    }
    catch(exception e)
    {
        combined_logger->error("rosAgv, processServiceCall exception...");
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

    combined_logger->info("rosAgv, startTask: " + task_name);

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
             combined_logger->error("rosAgv, start task: " +  task_name + "error...");
             return false;
         }
     }
     return true;
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

void rosAgv::stop()
{
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

    if(agv_path.size() > 0)
    {
        setAgvPath(agv_path);
    }
    else
    {
        combined_logger->error("rosAgv goStation no path...");
        return;
    }
    startTask(goal);
}


void rosAgv::arrve(int x,int y){

    /*for(auto station:excutestations){
        if(sqrt(pow(x-station->getX(),2)+pow(y-station->getY(),2))<20){
            onArriveStation(station);
            break;
        }
    }*/
}



//请求切换地图(呼叫电梯)
void rosAgv::callMapChange(int station)
//void rosAgv::callMapChange(AgvStationPtr station)
{
    //if(!station->getMapChangeStation())return ;
    //例如:
    //1楼电梯内坐标 (100,100)
    //2楼电梯内坐标 (200,200)
    //3楼电梯内坐标 (300,300)
    //给电梯一个到达1楼并开门的指令
    //    if(station->x == 100 && station->y == 100){
    //        elevactorGoFloor(1);
    //        elevactorOpenDoor();
    //    }
    //    else if(station->x == 200 && station->y == 200){
    //        elevactorGoFloor(2);
    //        elevactorOpenDoor();
    //    }
    //    else if(station->x == 300 && station->y == 300){
    //        elevactorGoFloor(3);
    //        elevactorOpenDoor();
    //    }else{
    //        //
    //        throw std::exception("地图切换站点错误");
    //    }

}


void rosAgv::excutePath(std::vector<int> lines)
{
    std::unique_lock <std::mutex> lock(nav_ctrl_status_mutex);

    stationMtx.lock();
    excutestations.clear();
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int endId = path->getEnd();
        excutestations.push_back(endId);
    }
    stationMtx.unlock();
    //告诉小车接下来要执行的路径
    goStation(lines, true);

    combined_logger->info("wait for task complete!!!!");

    nav_ctrl_status_var.wait(lock);

    combined_logger->info("task finished...................");

}

bool rosAgv::sendJsonToAGV(Json::Value json)
{
    bool result=true;

    std::string str_json = json.toStyledString();

    char *copy_temp = new char[str_json.size() + 1];
    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());
        if(!send((const char *)copy_temp, json.toStyledString().size()))
        {
            combined_logger->error("rosAgv, sendJsonToAGV failed, send data error...");
            result=false;
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

void rosAgv::setChipMounter(chipmounter* device)
{
    if(device != nullptr)
    {
        mChipmounter = device;
        combined_logger->info("rosAgv, setChipMounter success...");
    }
    else
    {
        combined_logger->error("rosAgv, setChipMounter failed...");
    }
}

bool rosAgv::beforeDoing(string ip, int port, string action, int station_id)
{
    int try_times = 0;
    std::chrono::milliseconds dura(200);

    if("none" == action)
        return true;
    else if("put" == action || "get" == action)
    {
        //if(station_id is chipmount_station )
        {
            mChipmounter = new chipmounter(1, "", ip, port);
            if(mChipmounter->init())
            {
                while(!mChipmounter->isConnected())
                {
                    std::this_thread::sleep_for(dura);
                    try_times ++;
                    if(try_times > 30)
                    {
                        combined_logger->error("rosAgv, beforeDoing, connect error...");

                        delete mChipmounter;
                        return false;
                    }
                }
                return true;
            }
            else
            {
                delete mChipmounter;
                return false;
            }

        }
    }
    return false;

}

bool rosAgv::Doing(string action, int station_id)
{
    std::chrono::milliseconds dura(20);
    int times = 0;

    combined_logger->info("rosAgv, . start doing..");


    //if(station_id is chipmount_station )
    {
        if("loading" == action )
        {
            combined_logger->info("rosAgv, .startShelftUp..");

            startShelftUp(action);

            combined_logger->info("rosAgv, .startShelftUp end..");


            if(mChipmounter == nullptr)
            {
                combined_logger->error("rosAgv,Doing, mChipmounter == nullptr...");
                return false;
            }

            combined_logger->info("rosAgv, startLoading...");

            mChipmounter->startLoading(station_id);

            combined_logger->info("rosAgv, startRolling...");

            //PLC start to rolling, need AGV also start to rolling
            startRolling(AGV_SHELVES_ROLLING_FORWORD);
            combined_logger->info("rosAgv, waitting for chipmounter loading finished...");

            while(!mChipmounter->isLoadingFinished())
            {
                std::this_thread::sleep_for(dura);
                /*times ++;
                if(times >= 50)
                    break;*/
            }
            stopRolling();

            startShelftDown(action);

            return true;
        }
        else if("unloading" == action)
        {
            combined_logger->info("rosAgv, .startShelftUp..");
            startShelftUp(action);
            combined_logger->info("rosAgv, .startShelftUp  end..");

            startRolling(AGV_SHELVES_ROLLING_BACKWORD);

            if(mChipmounter == nullptr)
            {
                combined_logger->error("rosAgv,Doing, mChipmounter == nullptr...");
                return false;
            }

            mChipmounter->startUnLoading(station_id);

            combined_logger->info("rosAgv, waitting for chipmounter unloading finished...");

            while(!mChipmounter->isUnLoadingFinished())
            {
                std::this_thread::sleep_for(dura);
                /*times ++;
                if(times >= 50)
                    break;*/
            }

            combined_logger->info("rosAgv,Doing, call stopRolling...");

            stopRolling();
            startShelftDown(action);

            combined_logger->info("rosAgv,Doing,unloading end...");

            return true;

        }
    }

    combined_logger->info("rosAgv,Doing, end...");

    return false;
}

bool rosAgv::afterDoing(string action, int station_id)
{

}

void rosAgv::startRolling(bool forword)
{
    /*std::unique_lock <std::mutex> lock(shelf_status_mutex);
    if(forword)
        startTask("roll_forword");
    else
        startTask("roll_backword");
    nav_ctrl_status_var.wait(lock);
    */

    Json::Value msg;

    if(forword)
    {
        msg["data"]="load_all:1" ;
    }
    else
    {
        msg["data"]="load_all:2";
    }

    publishTopic("/waypoint_user_pub", msg);
}
void rosAgv::stopRolling()
{
    Json::Value msg;

    msg["data"]="load_all:0" ;

    publishTopic("/waypoint_user_pub", msg);}

void rosAgv::startShelftUp(string action)
{
    /*combined_logger->info("rosAgv, startTask startShelftUp...");

    std::unique_lock <std::mutex> lock(shelf_status_mutex);

    combined_logger->info("rosAgv, startTask lift_up_900...");

    startTask("lift_up_900");

    combined_logger->info("rosAgv, lift_up_900 wait lock...");

    nav_ctrl_status_var.wait(lock);

    combined_logger->info("rosAgv, lift_up_900 end...");
    */
    if(action == "loading")
    {
        ControlShelfUpDown(3, "87500");
        combined_logger->info("rosAgv, 3, 87500.end..................");

        sleep(1);
        combined_logger->info("rosAgv, 2, 63000...");

        ControlShelfUpDown(2, "63000");

        combined_logger->info("rosAgv, 2, 63000. end...............");

        sleep(1);

        combined_logger->info("rosAgv, 40000...");

        ControlShelfUpDown(1, "40000");
        sleep(10);
    }
    else if(action == "unloading")
    {
        ControlShelfUpDown(3, "86000");
        combined_logger->info("rosAgv, 3, 86000.end..................");

        sleep(1);
        combined_logger->info("rosAgv, 2, 62000...");

        ControlShelfUpDown(2, "62000");

        combined_logger->info("rosAgv, 2, 62000. end...............");

        sleep(1);

        combined_logger->info("rosAgv, 37500...");

        ControlShelfUpDown(1, "37500");
        sleep(10);
    }

}

void rosAgv::startShelftDown(string action)
{
    /*std::unique_lock <std::mutex> lock(shelf_status_mutex);

    startTask("lift_down_0");
    nav_ctrl_status_var.wait(lock);
    */
    ControlShelfUpDown(1, "0");
    combined_logger->info("rosAgv, 1, 00000.end..................");

    sleep(1);
    combined_logger->info("rosAgv, 2, 20000...");

    ControlShelfUpDown(2, "20000");

    combined_logger->info("rosAgv, 2, 20000. end...............");

    sleep(1);

    combined_logger->info("rosAgv, 40000...");

    ControlShelfUpDown(3, "40000");
    sleep(1);
}

void rosAgv::test()
{
    std::vector<int> lines;
    int path0=6;
    int path1=7;
    int path2=8;

    sleep(3);

    lines.push_back(path0);
    lines.push_back(path1);
    lines.push_back(path2);

    excutePath(lines);
}

void rosAgv::test2()
{
    std::vector<int> lines;
    int path0=10;
    int path1=7;
    int path2=8;

    sleep(15);

    lines.push_back(path0);
    lines.push_back(path1);
    lines.push_back(path2);

    excutePath(lines);
}


void rosAgv::startTask(string station, string action)
{
    int16_t station_id;
    std::unique_lock <std::mutex> lock(nav_ctrl_status_mutex);

    combined_logger->info("rosAgv, startTask ");

    if(station == "2510")
    {
        station_id=0x2510;
        combined_logger->info("rosAgv, startTask polar 06");
    }
    else if(station == "2511")
    {
        station_id=0x2511;
        combined_logger->info("rosAgv, polar 07 ");
    }

    if(action == "unloading")
    {
        combined_logger->info("rosAgv,  取空卡塞");
        startTask("lift_polar_" + station);

        combined_logger->info("rosAgv, wait lock...");

        nav_ctrl_status_var.wait(lock);

        combined_logger->info("rosAgv, call doing...");

        Doing(action, station_id);

        combined_logger->info("rosAgv,  doing end...");

        startTask("lift_polar_back");

        combined_logger->info("rosAgv,  startTask lift_polar_back...");


        nav_ctrl_status_var.wait(lock);
        combined_logger->info("rosAgv, startTask 取空卡塞 end.....");
    }
    else if(action == "loading")
    {      
        combined_logger->info("rosAgv, polar 07  上料");
        startTask("lift_polar_" + station);

        combined_logger->info("rosAgv, wait lock...");

        nav_ctrl_status_var.wait(lock);

        combined_logger->info("rosAgv, start doing..");

        Doing(action, station_id);

        combined_logger->info("rosAgv, start lift_polar_back..");

        startTask("lift_polar_back");
        nav_ctrl_status_var.wait(lock);

    }

    combined_logger->info("rosAgv, task end haha..");

}




void rosAgv::ControlShelfUpDown(int layer, string height)
{
    Json::Value msg;

    if(layer == 1)
    {
        msg["data"]="elevate:10," + height;
    }
    else if(layer == 2)
    {
        msg["data"]="elevate:12," + height;
    }
    else if(layer == 3)
    {
        msg["data"]="elevate:14," + height;
    }

    publishTopic("/waypoint_user_pub", msg);
}


void rosAgv::InitShelfLayer()
{
    if(m_bInitlayer == false)
    {
        /*combined_logger->info("rosAgv, InitShelfLayer...");

        //std::unique_lock <std::mutex> lock(shelf_status_mutex);

        combined_logger->info("rosAgv, InitShelfLayer 222...");

        startTask("init_layers");

        combined_logger->info("rosAgv, InitShelfLayer lock...");

        //nav_ctrl_status_var.wait(lock);

        combined_logger->info("rosAgv, InitShelfLayer end...");

        sleep(20);*/

        startShelftDown("");

        m_bInitlayer = true;
    }
    else
    {
        combined_logger->info("rosAgv, InitShelfLayer m_bInitlayer is true, no need to init...");
        /*ControlShelfUpDown(1, "0");
        ControlShelfUpDown(2, "0");
        ControlShelfUpDown(3, "0");
        */
        startShelftDown("");
    }

    /*sleep(5);

    combined_logger->info("rosAgv, 3, 86500...");


    ControlShelfUpDown(3, "86500");
    combined_logger->info("rosAgv, 3, 87000.end..................");

    sleep(10);
    combined_logger->info("rosAgv, 2, 62500...");

    ControlShelfUpDown(2, "62500");

    combined_logger->info("rosAgv, 2, 62500. end...............");

    sleep(10);

    combined_logger->info("rosAgv, 40500...");

    ControlShelfUpDown(1, "40500");

    combined_logger->info("rosAgv, 40500...end..............");


    combined_logger->info("rosAgv, startRolling sleep...");

    sleep(3);
    startRolling(true);
    */

    //sleep(30);

    //combined_logger->info("rosAgv, stopRolling...");

    //stopRolling();

}


bool rosAgv::isAGVInit()
{
    return m_bInitlayer;
}
