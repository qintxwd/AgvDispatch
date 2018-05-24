#include "rosAgv.h"
#include "../../../mapmap/mapmanager.h"





rosAgv::rosAgv(int id, std::string name, std::string ip, int port):
    Agv(id,name,ip,port)
{

}


void rosAgv::onRead(const char *data,int len)
{
    //combined_logger->info("rosAgv onRead, data arrvied, len" + len);
    combined_logger->info("rosAgv onRead, data : " + string(data));
}

void rosAgv::onConnect()
{
    combined_logger->info("rosAgv onConnect OK! name: "+getName());
    //subTopic((getName() + AGV_POSE_TOPIC_NAME).c_str(), AGV_POSE_TOPIC_TYPE);
    if(NAV_CTRL_USING_TOPIC)
    {
        advertiseTopic((getName() + "/nav_ctrl").c_str(), "yocs_msgs/NavigationControl");
    }
    test();
}

void rosAgv::onDisconnect()
{

}

void rosAgv::subTopic(const char * topic, const char * topic_type)
{
    Json::Value json;
    json["op"]="subscribe";
    json["topic"]=topic;
    json["type"]=topic_type;

    std::string str_json = json.toStyledString();
    char *copy_temp = new char[str_json.size() + 1];
    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());
        if(!send((const char *)copy_temp, json.toStyledString().size()))
        {
            combined_logger->error("rosAgv,sub topic failed, send data fail...");
        }

        delete[] copy_temp;
    }
    else
        combined_logger->error("rosAgv,sub topic error...");
}

void rosAgv::advertiseTopic(const char * topic, const char * topic_type)
{
    Json::Value json;
    json["op"]="advertise";
    json["topic"]=topic;
    json["type"]=topic_type;

    std::string str_json = json.toStyledString();
    char *copy_temp = new char[str_json.size() + 1];
    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());
        if(!send((const char *)copy_temp, json.toStyledString().size()))
        {
            combined_logger->error("rosAgv, advertise topic failed, send data fail...");
        }
        delete[] copy_temp;
    }
    else
         combined_logger->error("rosAgv, advertise topic error...");
}

void rosAgv::publishTopic(const char * topic, Json::Value msg)
{
    Json::Value json;
    json["op"]="publish";
    json["topic"]=topic;
    json["msg"]=msg;

    std::string str_json = json.toStyledString();
    char *copy_temp = new char[str_json.size() + 1];
    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());
        if(!send((const char *)copy_temp, json.toStyledString().size()))
        {
            combined_logger->error("rosAgv, publishTopic topic failed, send data fail...");
        }
        delete[] copy_temp;
    }
    else
        combined_logger->error("rosAgv, publishTopic topic error...");

}

bool rosAgv::startTask(std::string task_name)
{
     if(NAV_CTRL_USING_TOPIC)
     {
         Json::Value msg;
         msg["goal_name"]=task_name;
         msg["control"]=START;
         publishTopic((getName() + "/nav_ctrl").c_str(), msg);
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
         json["service"]=getName() + "/nav_ctrl_service";
         json["args"]=nav_ctrl_srv;

         std::string str_json = json.toStyledString();

         char *copy_temp = new char[str_json.size() + 1];
         if(copy_temp != NULL)
         {
             strcpy(copy_temp, str_json.c_str());
             if(!send((const char *)copy_temp, json.toStyledString().size()))
             {
                 combined_logger->error("rosAgv, start task " + task_name + " failed, send data error...");
                 delete[] copy_temp;
                 return false;
             }
             delete[] copy_temp;
         }
         else
         {
             combined_logger->error("rosAgv, start task " + task_name + " error");
             return false;
         }
     }
     return true;
}

void rosAgv::cancelTask()
{

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
    json["service"]=getName() + "/set_planner_path";
    json["args"]=msg;

    std::string str_json = json.toStyledString();

    char *copy_temp = new char[str_json.size() + 1];
    if(copy_temp != NULL)
    {
        strcpy(copy_temp, str_json.c_str());
        if(!send((const char *)copy_temp, json.toStyledString().size()))
        {
            combined_logger->error("rosAgv, setAgvPath failed, send data error...");
        }
        delete[] copy_temp;
    }
    else
    {
        combined_logger->error("rosAgv, setAgvPath error...");
    }
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
}

void rosAgv::test()
{
    std::vector<int> lines;
    int path0=6;
    int path1=7;
    int path2=8;
    lines.push_back(path0);
    lines.push_back(path1);
    lines.push_back(path2);

    excutePath(lines);
}

