#ifndef ROSAGV_H
#define ROSAGV_H

#include "../../../agv.h"
#include "../../../agvtask.h"
#include "../../../network/tcpclient.h"
#include "../../../userlogmanager.h"
#include "../../../msgprocess.h"
#include "linepath.h"
#include "device/device.h"
#include "qunchuang/chipmounter/chipmounter.h"
#include "qunchuang/qunchuangtcsconnection.h"
#include "realagv.h"
#include "device/elevator/elevatorManager.h"


using namespace std;

#define SIMULATOR 0

#define NAV_CTRL_USING_TOPIC  0

#if SIMULATOR
#define AGV_POSE_TOPIC_NAME  "/base_pose_ground_truth"
#define AGV_POSE_TOPIC_TYPE  "nav_msgs/Odometry"
#else
#define AGV_POSE_TOPIC_NAME  "/robot_pose"
#define AGV_POSE_TOPIC_TYPE  "geometry_msgs/Pose"
#endif

#define INNOLUX_DB_BRANCH_SH1 "INNOLUX-SH1-"
#define INIT_AGV_POSE_OUT_ELEVATOR_NAME   "init_pose_"




class rosAgv;
using rosAgvPtr = std::shared_ptr<rosAgv>;

//如下 NAV_CONTROL define 是在 AGV中定义, 不能随便更改
enum nav_control{
    STOP,
    START,
    PAUSE
};

//3层升降货架上料status
enum up_part_status{
    up_part_working,  //0:正在执行;
    up_part_nothing,  //1:上料没料;
    up_part_fail,     //2:上料卡料;
    up_part_success   //3:上料成功
};
//3层升降货架下料status
enum down_part_status{
    down_part_working,  //0:正在执行;
    down_part_nothing=11,  //11:未收到料;
    down_part_fail=12,     //12:下料卡料;
    down_part_success=13   //13:下料成功
};


#define UP_PART      true  //3层升降货架上料
#define DOWN_PART    false //3层升降货架下料



#define NAV_CTRL_STATUS_ERROR      -1
#define NAV_CTRL_STATUS_IDLING     0
#define NAV_CTRL_STATUS_RUNNING    1
#define NAV_CTRL_STATUS_PAUSED     2
#define NAV_CTRL_STATUS_COMPLETED  3
#define NAV_CTRL_STATUS_CANCELLED  4
#define NAV_CTRL_STATUS_SUB_CANCELLED  5
//如上 NAV_CONTROL define 是在 AGV中定义, 不能随便更改

//AGV 货架转动方向
#define AGV_SHELVES_ROLLING_FORWORD true  // 向前转， AGV方向
#define AGV_SHELVES_ROLLING_BACKWORD false // 向后转

#define AGV_ACTION_PUT   "put"    // AGV向偏贴机上料
#define AGV_ACTION_GET "get"  // AGV从偏贴机下料
#define AGV_ACTION_NONE       "none"       //无动作


#define AGV_SHELVES_1_FLOOR_HEIGHT  39000
#define AGV_SHELVES_2_FLOOR_HEIGHT  64000
#define AGV_SHELVES_3_FLOOR_HEIGHT  87500

#define AGV_SHELVES_1_FLOOR_INIT_HEIGHT  0
#define AGV_SHELVES_2_FLOOR_INIT_HEIGHT  20000
#define AGV_SHELVES_3_FLOOR_INIT_HEIGHT  40000

class rosAgv : public RealAgv
{
private:
       //geometry_msgs::Pose agvPose;
       std::mutex parseDataMtx;

       std::mutex nav_ctrl_status_mutex;
       std::mutex shelf_status_mutex;

       std::condition_variable nav_ctrl_status_var; // nav_ctrl_status条件变量.

       void subTopic(const char * topic, const char * topic_type);
       void unSubTopic(const char * topic);
       void advertiseTopic(const char * topic, const char * topic_type);
       void advertiseService(const char * service_name, const char * msg_type);

       void publishTopic(const char * topic, Json::Value msg);
       void parseJsondata(const char *data,int len);
       void processServiceCall(Json::Value call_service);
       void processServiceResponse(Json::Value response);
       void processPublishMsg(Json::Value json);

       void sendServiceResponse(string service_name,Json::Value *value=nullptr,string id="");
       void navCtrlStatusNotify(string waypoint_name, int nav_ctrl_status);
       void changeMap(string map_name);
       void startRolling(bool forword);//send to 偏贴机AGV start rolling topic
       void stopRolling();//send to 偏贴机AGV stop rolling topic

       void startShelftUp(string action);
       void startShelftDown(string action);
       void initStation(string station_name);
       bool isShelftSuccess(bool forward);//判断3层升降货架status, 上下料成功true, 卡料false
       void reportToTCSStation(int station_id, bool arrived);// arrvied: true, leave: false
       string getStationNum(string stationName);
       int getStationId(string station_name);
       bool isNeedTakeElevator(std::vector<int> lines);
       std::vector<int> getPathToElevator(std::vector<int> lines);
       std::vector<int> getPathEnterElevator(std::vector<int> lines);
       std::vector<int> getPathOutElevator(std::vector<int> lines);
       std::vector<int> getPathLeaveElevator(std::vector<int> lines);
       std::vector<int> getPathInElevator(std::vector<int> lines);
       void setRebootState();


public:
    rosAgv(int id,std::string name,std::string ip,int port,int agvType=-1, int agvClass=-1, std::string lineName="");

    enum { Type = Agv::Type+1 };

    int type(){return Type;}

    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();

    bool startTask(std::string task_name);
    virtual void cancelTask();
    //virtual void excutePath(std::vector<AgvLinePtr> lines);
    virtual void excutePath(std::vector<int> lines);

    bool beforeDoing(string ip, int port, string action, string station);
    bool Doing(string action, int station_id);
    bool afterDoing(string action, int station_id);
    void setChipMounter(chipmounter* device);// only for test, will be removed
    bool secondaryLocalization(MapPoint* mapPoint);
    bool secondaryLocalization(string station);

    bool isAGVInit();
    void onTaskStart(AgvTaskPtr _task);
    void onTaskFinished(AgvTaskPtr _task);

    lynx::Msg getTcsMsg();

    void setAgvType(int type){agvType=type;}
    int getAgvType(){return agvType;}

    void setAgvClass(int _agvClass){agvClass=_agvClass;}
    int getAgvClass(){return agvClass;}

    void setLineName(std::string name){lineName=name;}
    std::string getLineName(){return lineName;}

private:
    virtual void arrve(int x,int y);
    //void goStation(AgvStationPtr station, bool stop = false, std::vector<AgvLinePtr> lines);
    //void goStation(AgvStationPtr station, AgvLinePtr line, bool stop = false);
    //void goStation(AgvStationPtr station, std::vector<AgvLinePtr> lines,  bool stop);
    //void goStation(int station, AgvLinePtr line, bool stop = false);
    void goStation(std::vector<int> lines,  bool stop);
    //virtual void goStation(int station, bool stop = false);
    virtual void stop();
    virtual void callMapChange(int station);
    void setAgvPath(std::vector<Pose2D> path);
    bool sendJsonToAGV(Json::Value json);

    //nav ctrl status 状态
    int m_nav_ctrl_status;

    chipmounter* mChipmounter; //偏贴机

    bool m_bInitlayer;
    bool bAgvConnected;


    int agvClass; //激光叉车, 激光AGV, 磁条AGV, 二维码AGV
    int agvType; //AGV type;
    std::string lineName; //agv对应产线name, 没有可以忽略

    void InitShelfLayer();
    void ControlShelfUpDown(int layer, string height);

    bool send(const char *data, int len);



    /*3层升降货架status
     * 上料 up_part_status:3,3,3.  0:正在执行; 1:上料没料; 2:上料卡料; 3:上料成功
     * 下料 down_part_status:13,13,13.  0:正在执行; 11:未收到料; 12:下料卡料; 13:下料成功
     */
    std::string shelf_finished_status;

    std::string last_tcp_json_data; //onread函数中上次获得数据, 需要连接处理


};

#endif // ROSAGV_H
