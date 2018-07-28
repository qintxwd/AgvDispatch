#ifndef DYFORKLIFT_H
#define DYFORKLIFT_H

#include "../agv.h"
#include "../agvtask.h"
#include "../network/sessionmanager.h"

#define PRECISION 20
#define START_RANGE 300
#define PRECMD_RANGE 500
#define MAX_WAITTIMES 10
class DyForklift;
using DyForkliftPtr = std::shared_ptr<DyForklift>;

enum FORKLIFT_COMM
{
    FORKLIFT_BATTERY = 25,
    FORKLIFT_POS = 29,
    FORKLIFT_FINISH = 26,
    FORKLIFT_STARTREPORT = 21,
    FORKLIFT_ENDREPORT = 22,
    FORKLIFT_MOVE = 35,
    FORKLIFT_FORK = 67,
    FORKLIFT_TURN = 30,
    FORKLIFT_MOVE_NOLASER = 37,
    FORKLIFT_HEART = 9
};

enum FORKLIFT_FORKPARAMS
{
  FORKLIFT_UP = 11,
  FORKLIFT_DOWN = 00
};

class DyForklift : public Agv
{
public:
    DyForklift(int id,std::string name,std::string ip,int port);

    enum { Type = Agv::Type+2 };

    int type(){return Type;}

    void onRead(const char *data,int len);
    void onRecv(const char *data,int len);

    void excutePath(std::vector<int> lines);
    void goStation(std::vector<int> lines,  bool stop);
    bool goElevator(int startStation,  int endStation, int from, int to, int eleID);
    void setQyhTcp(qyhnetwork::TcpSocketPtr _qyhTcp);

    bool startReport(int interval);
    bool endReport();
    bool turn(float speed, float angle);    //车辆旋转//返回发送是否成功
    bool waitTurnEnd(int waitMs);    //等待旋转结束，如果接收失败，返回false。返回接受的结果
    bool fork(int params); //1-liftup 0-setdown
    bool move(float speed, float distance);//进电梯时用
    bool heart();
    bool isFinish();
    bool isFinish(int cmd_type);

    Pose4D getPos();
    int nearestStation(int x, int y, int floor);
    void arrve(int x,int y);

    void onTaskStart(AgvTaskPtr _task);
    void onTaskFinished(AgvTaskPtr _task);

    ~DyForklift(){}
private:
    static const int maxResendTime = 10;

    bool resend(const char *data, int len);
    bool send(const char *data, int len);

    void init();
    bool turnResult;

    Pose4D m_currentPos;
    int m_power;
    bool m_lift = false;
    std::map<int,  DyMsg> m_unRecvSend;
    std::map<int,  DyMsg> m_unFinishCmd;
    std::mutex msgMtx;

    int actionpoint, startpoint;
    bool actionFlag = false;
    bool startLift = false;
    int task_type;

    qyhnetwork::TcpSocketPtr m_qTcp;
};

#endif // DYFORKLIFT_H
