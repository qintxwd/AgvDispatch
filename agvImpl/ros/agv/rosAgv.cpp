#include "rosAgv.h"
#include "mapmap/mappoint.h"
//#include <locale>
#include <condition_variable>
#include "rosAgvUtils.h"

rosAgv::rosAgv(int id, std::string name, std::string ip, int port,int agvType, int agvClass, std::string lineName):
    RealAgv(id,name,ip,port/*,agvType,agvClass,lineName*/)
{
    mChipmounter = nullptr;

    m_bInitlayer = false;
    this->agvType = AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF;

    shelf_finished_status="";
    last_tcp_json_data="";

    //每个AGV需要根据原点修改
    initStation("station_2000"); //初始化站点, 通常为AGV原点
}

void rosAgv::onConnect()
{
    combined_logger->info("rosAgv onConnect OK! name: "+getName());
#if SIMULATOR
    //subTopic((getName() + AGV_POSE_TOPIC_NAME).c_str(), AGV_POSE_TOPIC_TYPE);
    subTopic((getName() + "/rosnodejs/shell_feedback").c_str(), "std_msgs/String");
#else
    //subTopic(AGV_POSE_TOPIC_NAME, AGV_POSE_TOPIC_TYPE);
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
        advertiseTopic("/waypoint_user_pub", "std_msgs/String");

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
        std::vector<std::string>  jsons = splitMultiJson(last_tcp_json_data + string(data));

        //combined_logger->info("data: "+ string(data));

        for(int i=0; i<jsons.size(); i++)
        {
            std::string json = jsons.at(i);

            const char * data = json.c_str();
            if(*data == '{' && *(data + (json.size()-1))== '}')
            {
                last_tcp_json_data="";
                parseJsondata(json.c_str(),len);
            }
            else
            {
                last_tcp_json_data=json;
                //combined_logger->error("last_tcp_json_data: " + json);
            }
        }

        parseDataMtx.unlock();
    }
    catch(exception e)
    {
        combined_logger->error("rosAgv onRead, exception" );
    }
}




//判断3层升降货架status, 上下料成功true, 卡料false
bool rosAgv::isShelftSuccess(bool forward)
{
    //3层升降货架向前转
    if(forward)
    {
        if(shelf_finished_status.find("up_part_status") != string::npos)
        {
            if(shelf_finished_status.find("2") == string::npos)
            {
                combined_logger->info("3层升降货架status: 上料成功");
                shelf_finished_status = "";
                return true;
            }
            else
            {
                combined_logger->info("3层升降货架status: 上料卡料");
                shelf_finished_status = "";
                return false;
            }
        }
        else
        {
            combined_logger->error("3层升降货架status: 无信息");
            return false;
        }
    }
    else//3层升降货架向后转
    {
        if(shelf_finished_status.find("down_part_status") != string::npos)
        {
            if(shelf_finished_status.find("12") == string::npos)
            {
                combined_logger->info("3层升降货架status: 下料成功");
                shelf_finished_status = "";
                return true;
            }
            else
            {
                combined_logger->info("3层升降货架status: 下料卡料");
                shelf_finished_status = "";
                return false;
            }
        }
        else
        {
            combined_logger->error("3层升降货架status: 无信息");
            return false;
        }
    }
}


void rosAgv::stop()
{
}

void rosAgv::onTaskStart(AgvTaskPtr _task)
{
    if(_task != nullptr)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        combined_logger->info("rosAgv,  onTaskStart......");
        QunChuangTcsConnection::Instance()->taskStart(dispatch_id, this->getId());
    }
}

void rosAgv::onTaskFinished(AgvTaskPtr _task)
{
    combined_logger->info("rosAgv,  onTaskFinished......");
    if(_task != nullptr)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        QunChuangTcsConnection::Instance()->taskFinished(dispatch_id, this->getId(), true);

        //完成任务返回待命点, 以后需要添加到TaskManager
        string need_back_to_waiting = _task->getExtraParam("NEED_AGV_BACK_TO_WAITING_AREA_KEY");
        if(need_back_to_waiting == "true")
        {
            combined_logger->info("完成任务返回待命点, 以后需要添加到TaskManager......");
            //startTask("lift_polar_back");
        }
        else
        {
            sleep(10);
            combined_logger->info("rosAgv,  onTaskFinished, 不需要返回待命点");
        }
    }

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
    std::chrono::milliseconds dura(1000);

    combined_logger->error("rosAgv, beforeDoing..., action: {1}", action);

    if(AGV_ACTION_NONE == action)
        return true;
    else if(AGV_ACTION_PUT == action || AGV_ACTION_GET == action)
    {
        if(IsValidIPAddress(ip.c_str()))
        {
            combined_logger->info("rosAgv, beforeDoing...IsValidIPAddress");

            if(agvType == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)//3层升降货架AGV,  群创
            {

                combined_logger->info("rosAgv, beforeDoing...,agvType == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF");

                //startShelftUp(action);
                secondaryLocalization(intToString(station_id));

                combined_logger->error("rosAgv, new chipmounter.....");


                mChipmounter = new chipmounter(1, "", ip, port);
                if(mChipmounter->init())
                {
                    while(!mChipmounter->isConnected())
                    {
                        std::this_thread::sleep_for(dura);
                        try_times ++;
                        if(try_times > 600)
                        {
                            combined_logger->error("rosAgv, beforeDoing, connect error, delete mChipmounter...");

                            delete mChipmounter;
                            mChipmounter=nullptr;
                            return false;
                        }
                    }
                    return true;
                }
                else
                {
                    combined_logger->error("rosAgv, beforeDoing, mChipmounter init error, delete mChipmounter...");
                    delete mChipmounter;
                    mChipmounter=nullptr;
                    return false;
                }

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

    /*if(station_id == 2510)
    {
        station_id=0x2510;
        combined_logger->info("rosAgv, polar 06");
    }
    else if(station_id == 2511)
    {
        station_id=0x2511;
        combined_logger->info("rosAgv, polar 07 ");
    }*/

    if(currentTask == nullptr)
    {
        combined_logger->error("rosAgv, start doing, currentTask == nullptr");
        return false;
    }

    //if(station_id is chipmount_station )
    if(agvType == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)//3层升降货架AGV,  群创
    {
        if(AGV_ACTION_PUT == action )
        {
            startShelftUp(action);

            if(mChipmounter == nullptr)
            {
                combined_logger->error("rosAgv,Doing, mChipmounter == nullptr...");
                return false;
            }

            int all_floor_info = stringToInt(currentTask->getExtraParam(ALL_FLOOR_INFO_KEY));

            combined_logger->error("rosAgv, startLoading, all_floor_info: " + intToString(all_floor_info));

            mChipmounter->startLoading(station_id, all_floor_info);

            subTopic("/waypoint_user_sub", "std_msgs/String");

            //PLC start to rolling, need AGV also start to rolling
            startRolling(AGV_SHELVES_ROLLING_FORWORD);
            combined_logger->info("rosAgv, 等待上料完成...");

            startTask("get_up_part_status");

            while(!mChipmounter->isLoadingFinished())//等待上料完成
            {
                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("rosAgv, 上料完成...");


            while(m_nav_ctrl_status != NAV_CTRL_STATUS_COMPLETED)
            {
                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("停止订阅货架status");
            unSubTopic("/waypoint_user_sub"); //停止订阅货架status


            if(isShelftSuccess(UP_PART))
            {
                startShelftDown(action);
                return true;
            }
            else
            {
                combined_logger->error("rosAgv,上料卡料...");
                return false;
            }
        }
        else if(AGV_ACTION_GET == action)
        {
            startShelftUp(action);
            subTopic("/waypoint_user_sub", "std_msgs/String");
            startRolling(AGV_SHELVES_ROLLING_BACKWORD);

            if(mChipmounter == nullptr)
            {
                combined_logger->error("rosAgv,Doing, mChipmounter == nullptr...");
                return false;
            }

            mChipmounter->startUnLoading(station_id);
            startTask("get_down_part_status");
            combined_logger->info("rosAgv, 等待下料完成...");

            while(!mChipmounter->isUnLoadingFinished())//等待下料完成
            {
                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("rosAgv,下料完成...");


            while(m_nav_ctrl_status != NAV_CTRL_STATUS_COMPLETED)
            {
                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("停止订阅货架status");
            unSubTopic("/waypoint_user_sub"); //停止订阅货架status

            if(isShelftSuccess(DOWN_PART))
            {
                startShelftDown(action);
                combined_logger->info("rosAgv,Doing,unloading end...");
                return true;
            }
            else
            {
                combined_logger->error("rosAgv,下料卡料...");
                return false;
            }
        }
    }

    combined_logger->info("rosAgv,Doing, end...");

    return false;
}

bool rosAgv::afterDoing(string action, int station_id)
{
    if(mChipmounter != nullptr)
    {
        combined_logger->error("rosAgv, afterDoing, delete mChipmounter...");

        delete mChipmounter;
        mChipmounter=nullptr;

        combined_logger->error("rosAgv, afterDoing, delete mChipmounter end...");
        startTask("dock_back"); // agv back

    }    
    //startTask("lift_polar_back");

    return true;
}

void rosAgv::startRolling(bool forword) //3层升降货架AGV
{
    if(forword)
        startTask("roll_forword");
    else
        startTask("roll_backword");
}
void rosAgv::stopRolling()//3层升降货架AGV
{
    Json::Value msg;

    msg["data"]="load_all:0" ;

    publishTopic("/waypoint_user_pub", msg);}

void rosAgv::startShelftUp(string action)//3层升降货架AGV
{
    if(action == AGV_ACTION_PUT)
    {
        ControlShelfUpDown(3, "87500");
        sleep(1);
        ControlShelfUpDown(2, "63000");
        sleep(1);
        ControlShelfUpDown(1, "39000");
        sleep(10);
    }
    else if(action == AGV_ACTION_GET)
    {
        ControlShelfUpDown(3, "85500");
        sleep(1);
        ControlShelfUpDown(2, "61000");
        sleep(1);
        ControlShelfUpDown(1, "37000");
        sleep(10);
    }

}

void rosAgv::startShelftDown(string action)//3层升降货架AGV
{
    ControlShelfUpDown(1, "0");
    sleep(1);
    ControlShelfUpDown(2, "20000");
    sleep(1);
    ControlShelfUpDown(3, "40000");
    sleep(1);
}


void rosAgv::startTask(string station, string action)//3层升降货架AGV test
{
    int16_t station_id;
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

    if(action == AGV_ACTION_GET)
    {
        combined_logger->info("rosAgv,  取空卡塞");
        startTask("lift_polar_" + station);
        if(m_nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED)//AGV已到达(下料到达)
        {
            combined_logger->info("rosAgv, AGV已到达(下料到达)");
            if(true == Doing(action, station_id))
            {
                startTask("lift_polar_back");
                combined_logger->info("rosAgv, startTask 取空卡塞 end.....");
            }
        }
        else
        {
            combined_logger->error("rosAgv, 取空卡塞 error ...");
        }
    }
    else if(action == AGV_ACTION_PUT)
    {      
        combined_logger->info("rosAgv, polar 07  上料");
        startTask("lift_polar_" + station);

        if(m_nav_ctrl_status == NAV_CTRL_STATUS_COMPLETED)//AGV已到达(上料到达)
        {
            combined_logger->info("rosAgv, AGV已到达(上料到达)");

            if(true == Doing(action, station_id))
            {
                combined_logger->info("rosAgv, start lift_polar_back..");
                startTask("lift_polar_back");
            }
        }
        else
        {
            combined_logger->error("rosAgv, 上料 error ...");
        }
    }
    combined_logger->info("rosAgv, task end haha..");
}




void rosAgv::ControlShelfUpDown(int layer, string height)//3层升降货架AGV
{
    Json::Value msg;

    if(layer == 1)//最下面一层
    {
        msg["data"]="elevate:10," + height;
    }
    else if(layer == 2)//中间层
    {
        msg["data"]="elevate:12," + height;
    }
    else if(layer == 3)//最高层
    {
        msg["data"]="elevate:14," + height;
    }

    publishTopic("/waypoint_user_pub", msg);
}


void rosAgv::InitShelfLayer()//3层升降货架AGV
{
}


bool rosAgv::isAGVInit()
{
    agvType = AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF;

    if(agvType == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
        return true;
    else
    {
        combined_logger->error("rosAgv,111 isAGVInit : false");
        return true;
    }
}

void rosAgv::initStation(string station_name)
{
    auto nowSpirit = MapManager::getInstance()->getMapSpiritByName(station_name);
    if(nowSpirit!=nullptr && nowSpirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point)
    {
        nowStation = nowSpirit->getId();
    }
    else
    {
        combined_logger->error("rosAgv, initStation: " + station_name + " error...");
    }
}
