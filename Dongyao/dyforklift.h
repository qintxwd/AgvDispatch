#ifndef DYFORKLIFT_H
#define DYFORKLIFT_H

#include "../agv.h"
#include "../agvtask.h"
#include "../network/sessionmanager.h"

#define PRECISION 30
#define ANGLE_PRECISION 30
#define START_RANGE 300
#define PRECMD_RANGE 500
#define MAX_WAITTIMES 5

class DyForklift;
using DyForkliftPtr = std::shared_ptr<DyForklift>;

enum FORKLIFT_COMM
{
    FORKLIFT_BATTERY = 25,
    FORKLIFT_POS = 29,
    FORKLIFT_FINISH = 26,
    FORKLIFT_STARTREPORT = 21,
    FORKLIFT_ENDREPORT = 22,
    FORKLIFT_INITPOS = 30,
    FORKLIFT_MOVE = 35,
    FORKLIFT_MOVEELE = 36,
    FORKLIFT_STOP = 37,
    FORKLIFT_FORK = 67,
    FORKLIFT_TURN = 30,
    FORKLIFT_MOVE_NOLASER = 37,
    FORKLIFT_CHARGE = 69,
    FORKLIFT_HEART = 9,
    FORKLIFT_WARN = 24
};

enum FORKLIFT_FORKPARAMS
{
    FORKLIFT_UP = 11,
    FORKLIFT_DOWN = 00
};


enum FORKLIFT_CHARGEPARAMS
{
    FORKLIFT_START_CHARGE  = 1,
    FORKLIFT_END_CHARGE = 0
};
typedef struct WarnStatus
{
    int iLocateSt;            //定位丢失,1表示有告警，0表示无告警
    int iSchedualPlanSt;            //调度系统规划错误,1表示有告警，0表示无告警
    int iHandCtrSt;	            //模式切换（集中调度，手动），0表示只接受集中调度，1表示只接受手动控制
    int iObstacleLaserSt;          //激光避障状态 0： 远 1： 中 2：近
    int iScramSt;          //紧急停止状态 0:非 1：紧急停止
    int iTouchEdgeSt;      //触边状态    0：正常 1：触边
    int iTentacleSt;       //触须状态    0：正常  1：触须
    int iTemperatureSt;    //温度保护状态 0：正常  1：温度保护
    int iDriveSt;          //驱动状态    0：正常  1：异常
    int iBaffleSt;         //挡板状态    0：正常  1：异常
    int iBatterySt;        //电池状态    0：正常  1：过低
    //int Equipmment_Err_Status;          //设备故障告警
    int iLocationLaserConnectSt;        //定位激光连接状态    0： 正常 1:异常
    int iObstacleLaserConnectSt;        //避障激光连接状态    0： 正常 1：异常
    int iSerialPortConnectSt;           //串口连接状态       0： 正常 1：异常
    WarnStatus() //初始化
    {
        memset(this, 0, sizeof(WarnStatus));
    }
    bool operator ==(WarnStatus & t){
        if(iLocateSt == t.iLocateSt && iSchedualPlanSt == t.iSchedualPlanSt && iHandCtrSt == t.iHandCtrSt &&
                iObstacleLaserSt == t.iObstacleLaserSt && iScramSt == t.iScramSt && iTouchEdgeSt == t.iTouchEdgeSt &&
                iTentacleSt == t.iTentacleSt && iTemperatureSt == t.iTemperatureSt && iDriveSt == t.iDriveSt &&
                iBaffleSt == t.iBaffleSt && iBatterySt == t.iBatterySt && iLocationLaserConnectSt == t.iLocationLaserConnectSt &&
                iObstacleLaserConnectSt == t.iObstacleLaserConnectSt && iSerialPortConnectSt == t.iSerialPortConnectSt)
        {
            return true;
        }
        else
            return false;
    }

    std::string toString()
    {
        char buf[LOG_MSG_LENGTH];
        snprintf(buf, LOG_MSG_LENGTH, "定位丢失=%d 调度系统规划错误=%d 模式=%d 激光避障状态=%d 紧急停止状态=%d 触边状态=%d 触须状态=%d 温度保护状态=%d 驱动状态=%d 挡板状态=%d 电池状态=%d 定位激光连接状态=%d 避障激光连接状态=%d 串口连接状态=%d",
                            iLocateSt, iSchedualPlanSt, iHandCtrSt,iObstacleLaserSt,iScramSt, iTouchEdgeSt, iTentacleSt, iTemperatureSt, iDriveSt, iBaffleSt, iBatterySt, iLocationLaserConnectSt,iObstacleLaserConnectSt,iSerialPortConnectSt
                            );
        return std::string(buf);
    }
}WarnSt;
class DyForklift : public Agv
{
public:
    DyForklift(int id,std::string name,std::string ip,int port);

    enum { Type = Agv::Type+2 };

    int type(){return Type;}

    void onRead(const char *data,int len);

    void excutePath(std::vector<int> lines);
    void goStation(std::vector<int> lines,  bool stop, FORKLIFT_COMM cmd);
    void goElevator(std::vector<int> lines);
    void setQyhTcp(SessionPtr _qyhTcp);

    bool startReport(int interval);
    bool endReport();
    bool fork(int params); //1-liftup 0-setdown
    //bool move(float speed, float distance);//进电梯时用
    bool charge(int params);  //1-充电 0-退出充电
    //bool heart();
    bool isFinish();
    bool isFinish(int cmd_type);
    bool stopEmergency(int params);

    Pose4D getPos();
    int nearestStation(int x, int y, int a, int floor);
    void arrve(int x,int y);
    bool setInitPos(int station);

    void onTaskStart(AgvTaskPtr _task);
    void onTaskFinished(AgvTaskPtr _task);

    void goSameFloorPath(std::vector<int> lines);

    //电梯逻辑
    bool isNeedTakeElevator(std::vector<int> lines);
    std::vector<int> getPathToElevator(std::vector<int> lines);
    std::vector<int> getPathLeaveElevator(std::vector<int> lines);
    std::vector<int> getPathInElevator(std::vector<int> lines);
    std::vector<int> getPathEnterElevator(std::vector<int> lines);
    std::vector<int> getPathOutElevator(std::vector<int> lines);

    ~DyForklift(){}
	
	virtual bool pause();
    virtual bool resume();
private:
    static const int maxResendTime = 10;

    bool resend(const std::string &msg);
    bool send(const std::string &msg);

    void init();
    bool turnResult;

    Pose4D m_currentPos;
    int m_power;
    bool m_lift = false;
    std::map<int,  DyMsg> m_unRecvSend;
    std::map<int,  DyMsg> m_unFinishCmd;
    std::mutex msgMtx;

//    int actionpoint, startpoint;
//    bool actionFlag = false;
//    bool startLift = false;
    int task_type;

    SessionPtr m_qTcp;
    WarnSt m_warn;
	bool pausedFlag;//是否暂停了
    bool sendPause;//发送的是暂停的指令，还是继续的指令
};

#endif // DYFORKLIFT_H
