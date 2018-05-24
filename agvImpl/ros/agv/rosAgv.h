#ifndef ROSAGV_H
#define ROSAGV_H

#include "../../../agv.h"
#include "../../../agvtask.h"
#include "../../../qyhtcpclient.h"
#include "../../../userlogmanager.h"
#include "../../../msgprocess.h"
#include "linepath.h"

using namespace std;

class rosAgv;
using rosAgvPtr = std::shared_ptr<rosAgv>;

#define NAV_CTRL_USING_TOPIC  0

#define AGV_POSE_TOPIC_NAME  "/base_pose_ground_truth"
#define AGV_POSE_TOPIC_TYPE  "nav_msgs/Odometry"

enum nav_control{
    STOP,
    START,
    PAUSE
};

class rosAgv : public Agv
{
private:
       //geometry_msgs::Pose agvPose;
       void subTopic(const char * topic, const char * topic_type);
       void advertiseTopic(const char * topic, const char * topic_type);
       void publishTopic(const char * topic, Json::Value msg);

public:
    rosAgv(int id,std::string name,std::string ip,int port);

    enum { Type = Agv::Type+1 };

    int type(){return Type;}

    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();

    bool startTask(std::string task_name);
    virtual void cancelTask();
    //virtual void excutePath(std::vector<AgvLinePtr> lines);
    virtual void excutePath(std::vector<int> lines);

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
    void test();
};

#endif // ROSAGV_H
