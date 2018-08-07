#include "rosAgv.h"
#include "../../../mapmap/mappoint.h"
//#include <locale>
#include <condition_variable>
#include "rosAgvUtils.h"
#include "qunchuang/qunchuangnodedoing.h"
#include "taskmanager.h"

rosAgv::rosAgv(int id, std::string name, std::string ip, int port,int agvType, int agvClass, std::string lineName):
    RealAgv(id,name,ip,port/*,agvType,agvClass,lineName*/)
{
    mChipmounter = nullptr;

    m_bInitlayer = false;

    this->agvType = agvType;

    if(this->agvType ==AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
        combined_logger->info("rosAgv, 3层升降货架 AGV" );
    else
        combined_logger->info("rosAgv, agvType : {0}",  agvType);

    shelf_finished_status="";
    last_tcp_json_data="";

    bAgvConnected = false;

    //每个AGV需要根据原点修改
    //initStation("station_20003"); //初始化站点, 通常为AGV原点
}

void rosAgv::setRebootState()//AGV reboot but server is still running, need reset some state
{
    if(mChipmounter != nullptr)
    {
        delete mChipmounter;
        mChipmounter = nullptr;
    }
    shelf_finished_status="";

    if(this->getTask() != nullptr)
    {
        int taskId = this->getTask()->getId();
        this->cancelTask();
        TaskManager::getInstance()->cancelTask(taskId);
        this->setTask(nullptr);
    }

    status == Agv::AGV_STATUS_IDLE;

    initStation("station_2600"); //初始化站点, 通常为AGV原点, need set by agv id or name

}

void rosAgv::onConnect()
{
    bAgvConnected = true;

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
    bAgvConnected = false;
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

/*
 * qunchuang staion like this "station_2500", this function remove "station_"
 */
std::string rosAgv::getStationNum(std::string stationName)
{
    std::string result;
    if (stationName.length() == 0)
        return result;

    size_t pos = stationName.find_first_of("_");
    if(pos == std::string::npos){
        return stationName;
    }

    result = stationName.substr(pos+1);
    return result;
}

void rosAgv::reportToTCSStation(int station_id, bool arrive)
{
    AgvTaskPtr _task = this->getTask();
    if(_task != nullptr)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        //获取站点：
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station_id);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            combined_logger->error("rosAgv, reportToTCSStation, station : {0} is not a station", station_id);
            return;
        }

        string station_name = getStationNum(spirit->getName());

        if(arrive)
        {
            combined_logger->info("rosAgv,  reportToTCSStation, arrvive station_name : {0}", station_name);
            QunChuangTcsConnection::Instance()->reportArrivedStation(dispatch_id, this->getId(), station_name);
        }
        else
        {
            combined_logger->info("rosAgv,  reportToTCSStation, leave station_name : {0}", station_name);
            QunChuangTcsConnection::Instance()->reportLeaveStation(dispatch_id, this->getId(), station_name);
        }
    }
}

void rosAgv::onTaskStart(AgvTaskPtr _task)
{
    if(_task != nullptr)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        combined_logger->info("rosAgv,  onTaskStart, dispatch_id: {0}", dispatch_id);
        if(dispatch_id != " ")
            QunChuangTcsConnection::Instance()->taskStart(dispatch_id, this->getId());

        //回到等待区node, 强制返回二楼电梯旁边, 后面需要根据AGV和楼层动态设置
        //完成任务返回待命点, 以后需要添加到TaskManager
        string need_back_to_waiting = _task->getExtraParam("NEED_AGV_BACK_TO_WAITING_AREA_KEY");
        if(need_back_to_waiting == "true")
        {
            AgvTaskNodePtr waittingNode(new AgvTaskNode());
            waittingNode->setStation(getStationId("station_2600"));

            AgvTaskNodeDoThingPtr doThing(new QunChuangNodeDoing(std::vector<std::string>()));
            doThing->setStationId(getStationId("station_2600"));

            waittingNode->push_backDoThing(doThing);
            _task->push_backNode(waittingNode);
        }
    }

}

void rosAgv::onTaskFinished(AgvTaskPtr _task)
{
    combined_logger->info("rosAgv,  onTaskFinished......");
    /*if(_task != nullptr)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        QunChuangTcsConnection::Instance()->taskFinished(dispatch_id, this->getId(), true);

    }*/
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

bool rosAgv::beforeDoing(string ip, int port, string action, string station)
{
    int try_times = 0;
    std::chrono::milliseconds dura(1000);

    if(currentTask == nullptr)
    {
        combined_logger->error("rosAgv, beforeDoing, currentTask == nullptr");
        return false;
    }

    if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
    {
        combined_logger->error("rosAgv, beforeDoing, 强制结束任务");
        return true;
    }


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
                secondaryLocalization(station);

                combined_logger->error("rosAgv, new chipmounter.....");


                mChipmounter = new chipmounter(1, "", ip, port);
                if(mChipmounter->init())
                {
                    while(!mChipmounter->isConnected())
                    {
                        std::this_thread::sleep_for(dura);
                        try_times ++;

                        if(currentTask == nullptr)
                        {
                           combined_logger->debug("rosAgv, Doing, 任务取消");
                           return false;
                        }

                        if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
                        {
                            combined_logger->debug("rosAgv, Doing, 强制结束任务");
                            return false;
                        }
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

    if(currentTask == nullptr)
    {
        combined_logger->error("rosAgv, start doing, currentTask == nullptr");
        return false;
    }

    if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
    {
        combined_logger->error("rosAgv, Doing, 强制结束任务");
        return true;
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

            if(currentTask->getStatus() != Agv::AGV_STATUS_FORCE_FINISHED)
            {

                mChipmounter->startLoading(station_id, all_floor_info);

                subTopic("/waypoint_user_sub", "std_msgs/String");

                //PLC start to rolling, need AGV also start to rolling
                startRolling(AGV_SHELVES_ROLLING_FORWORD);
                combined_logger->info("rosAgv, 等待上料完成...");

                startTask("get_up_part_status");


                if(currentTask != nullptr)
                    combined_logger->info("rosAgv, currentTask status: {0}", currentTask->getStatus());


                //while(!mChipmounter->isLoadingFinished() && (currentTask != nullptr && currentTask->getStatus() != Agv::AGV_STATUS_FORCE_FINISHED))//等待上料完成
                while(!mChipmounter->isLoadingFinished())
                {
                    if(currentTask == nullptr)
                    {
                       combined_logger->debug("rosAgv, Doing, 任务取消");
                       return false;
                    }

                    if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
                    {
                        combined_logger->debug("rosAgv, Doing, 强制结束任务");
                        break;
                    }

                    combined_logger->info("rosAgv, 等待上料完成...");

                    std::this_thread::sleep_for(dura);
                }

                combined_logger->info("rosAgv, 上料完成...");


                if(currentTask != nullptr)
                    combined_logger->info("rosAgv, 222 currentTask status: {0}", currentTask->getStatus());

                //while(m_nav_ctrl_status != NAV_CTRL_STATUS_COMPLETED && (currentTask != nullptr && currentTask->getStatus() != Agv::AGV_STATUS_FORCE_FINISHED))
                while(m_nav_ctrl_status != NAV_CTRL_STATUS_COMPLETED)
                {
                    if(currentTask == nullptr)
                    {
                       combined_logger->debug("rosAgv, Doing, 任务取消");
                       break;
                    }

                    if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
                    {
                        combined_logger->debug("rosAgv, Doing, 强制结束任务");
                        break;
                    }

                    combined_logger->info("rosAgv, 等待货架status...");

                    std::this_thread::sleep_for(dura);
                }

                combined_logger->info("停止订阅货架status");
                unSubTopic("/waypoint_user_sub"); //停止订阅货架status
            }
            else
            {
                combined_logger->error("rosAgv, task force finised");
            }


            if(isShelftSuccess(UP_PART) || currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
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
                if(currentTask == nullptr)
                {
                   combined_logger->debug("rosAgv, Doing, 任务取消");
                   break;
                }

                if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
                {
                    combined_logger->debug("rosAgv, Doing, 强制结束任务");
                    break;
                }

                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("rosAgv,下料完成...");


            while(m_nav_ctrl_status != NAV_CTRL_STATUS_COMPLETED)
            {
                if(currentTask == nullptr)
                {
                   combined_logger->debug("rosAgv, Doing, 任务取消");
                   break;
                }

                if(currentTask->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
                {
                    combined_logger->debug("rosAgv, Doing, 强制结束任务");
                    break;
                }
                std::this_thread::sleep_for(dura);
            }

            combined_logger->info("停止订阅货架status");
            unSubTopic("/waypoint_user_sub"); //停止订阅货架status

            startShelftDown(action);

            if(isShelftSuccess(DOWN_PART))
            {
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
    bool taskFinished = false;

    if(mChipmounter != nullptr)
    {
        combined_logger->info("rosAgv, afterDoing, delete mChipmounter...");

        delete mChipmounter;
        mChipmounter=nullptr;

        taskFinished = true;

        combined_logger->info("完成任务返回待命点, 先后退一段距离......");
        startTask("dock_back");
    }

    AgvTaskPtr _task = this->getTask();
    if(_task == nullptr)
        return false;


    if(AGV_ACTION_PUT == action || _task->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED)
    {
        string dispatch_id = _task->getExtraParam("dispatch_id");

        if(this->agvType == AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF)
        {
            if(taskFinished || _task->getStatus() == Agv::AGV_STATUS_FORCE_FINISHED) //上料完成
            {
                QunChuangTcsConnection::Instance()->taskFinished(dispatch_id, this->getId(), true);
            }
            else  //下料完成, 等待人工取走空卡塞
            {
                combined_logger->debug("下料完成, 等待人工取走空卡塞......");

                while(true)
                {
                    if("true" == _task->getExtraParam("TASK_FINISHED_NOFITY"))
                    {
                        combined_logger->debug("人工取走空卡塞完成......");
                        QunChuangTcsConnection::Instance()->taskFinished(dispatch_id, this->getId(), true);

                        break;
                    }
                    else
                        sleep(1);
                }
            }
        }
        else
            QunChuangTcsConnection::Instance()->taskFinished(dispatch_id, this->getId(), true);
    }
    else
    {
        this->status = AGV_STATUS_WAITTING_PUT;
    }

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
    publishTopic("/waypoint_user_pub", msg);
}

void rosAgv::startShelftUp(string action)//3层升降货架AGV
{
    if(action == AGV_ACTION_PUT)
    {
        ControlShelfUpDown(3, intToString(AGV_SHELVES_3_FLOOR_HEIGHT));
        sleep(1);

        ControlShelfUpDown(2, intToString(AGV_SHELVES_2_FLOOR_HEIGHT));
        sleep(1);

        ControlShelfUpDown(1, intToString(AGV_SHELVES_1_FLOOR_HEIGHT));
        sleep(10);
    }
    else if(action == AGV_ACTION_GET)
    {
        //ControlShelfUpDown(3, "85500");
        ControlShelfUpDown(3, intToString(AGV_SHELVES_3_FLOOR_HEIGHT-2000));
        sleep(1);

        //ControlShelfUpDown(2, "61000");
        ControlShelfUpDown(2, intToString(AGV_SHELVES_2_FLOOR_HEIGHT-2000));
        sleep(1);

        //ControlShelfUpDown(1, "37000");
        ControlShelfUpDown(1, intToString(AGV_SHELVES_1_FLOOR_HEIGHT-2000));
        sleep(10);
    }

}

void rosAgv::startShelftDown(string action)//3层升降货架AGV
{
    //ControlShelfUpDown(1, "0");
    ControlShelfUpDown(1, intToString(AGV_SHELVES_1_FLOOR_INIT_HEIGHT));
    sleep(1);

    //ControlShelfUpDown(2, "20000");
    ControlShelfUpDown(2, intToString(AGV_SHELVES_2_FLOOR_INIT_HEIGHT));
    sleep(1);

    //ControlShelfUpDown(3, "40000");
    ControlShelfUpDown(3, intToString(AGV_SHELVES_3_FLOOR_INIT_HEIGHT));
    sleep(1);
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
    //agvType = AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF;

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

int rosAgv::getStationId(string station_name)
{
    auto nowSpirit = MapManager::getInstance()->getMapSpiritByName(station_name);
    if(nowSpirit!=nullptr && nowSpirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point)
    {
        return nowSpirit->getId();
    }
    else
    {
        combined_logger->error("rosAgv, getStationId: " + station_name + " error...");
        return 0;
    }
}
