#ifndef AGV_H
#define AGV_H
#include <string>
#include <vector>
#include <memory>

class AgvLine;
using AgvLinePtr = std::shared_ptr<AgvLine>;

class AgvTask;

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class QyhTcpClient;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;
//AGV
class Agv:public std::enable_shared_from_this<Agv>
{
public:
    Agv(int id,std::string name,std::string ip,int port);

    void init();

    ~Agv();

    virtual void excutePath(std::vector<AgvLinePtr> lines);

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
    AgvStationPtr lastStation = nullptr;//上一个站点
    AgvStationPtr nowStation = nullptr;//当前所在站点
    AgvStationPtr nextStation = nullptr;//下一个站点

    void setTask(AgvTask *_task){currentTask = _task;}
    AgvTask *getTask(){return currentTask;}

    int getId(){return id;}
    std::string getName(){return name;}
    std::string getIp(){return ip;}
    int getPort(){return port;}

    void setName(std::string _name){name=_name;}
    void setIp(std::string _ip){ip=_ip;}
    void setPort(int _port){port=_port;}

    //回调
    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();

    void reconnect();
private:
    AgvTask *currentTask;
    int id;
    std::string name;
    std::string ip;
    int port;

    void goStation(AgvStationPtr station, bool stop = false);
    void callMapChange(AgvStationPtr station);

    QyhTcpClient *tcpClient;
};

#endif // AGV_H
