#ifndef AGV_H
#define AGV_H
#include <string>
#include <vector>


class AgvLine;
class AgvTask;
class AgvStation;
class QyhTcpClient;
//AGV
class Agv
{
public:
    Agv(int id,std::string name,std::string ip,int port);

    void init();

    ~Agv();

    virtual void excutePath(std::vector<AgvLine *> lines);

    //状态
    enum {
        AGV_STATUS_HANDING = -1,//手动模式中，不可用
        AGV_STATUS_IDLE = 0,//空闲可用
        AGV_STATUS_UNCONNECT = 1,//未连接
        AGV_STATUS_TASKING = 2,//正在执行任务
        AGV_STATUS_POWER_LOW = 3,//电量低
        AGV_STATUS_ERROR = 4,//故障
        AGV_STATUS_GO_CHARGING = 5,//返回充电中
        AGV_STATUS_CHARGING = 6,//正在充电
    };
    int status = AGV_STATUS_IDLE;

    //计算路径用的
    AgvStation* lastStation = 0;//上一个站点
    AgvStation* nowStation = 0;//当前所在站点
    AgvStation* nextStation = 0;//下一个站点

    void setTask(AgvTask *_task){currentTask = _task;}
    AgvTask *getTask(){return currentTask;}

    int getId(){return id;}

    //回调
    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();
private:
    AgvTask *currentTask;
    int id;
    std::string name;
    std::string ip;
    int port;

    void goStation(AgvStation *station, bool stop = false);
    void callMapChange(AgvStation *station);

    QyhTcpClient *tcpClient;
};

#endif // AGV_H
