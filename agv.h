#ifndef AGV_H
#define AGV_H
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>

class AgvTask;
using AgvTaskPtr = std::shared_ptr<AgvTask>;

class QyhTcpClient;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;
//AGV
class Agv:public std::enable_shared_from_this<Agv>
{
public:
    Agv(int id,std::string name,std::string ip,int port);

    void init();

    virtual ~Agv();

    enum { Type = 0 };

    virtual int type(){return Type;}

    virtual void excutePath(std::vector<int> lines);

    virtual void cancelTask();

    bool send(const char *data, int len);

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

    //状态
    enum {
        AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF = 0,//3层升降货架AGV,  群创
    };

    int status = AGV_STATUS_IDLE;

    //计算路径用的
    int lastStation = 0;//上一个站点
	int nowStation = 0;//当前所在站点
	int nextStation = 0;//下一个站点

    void setTask(AgvTaskPtr _task){currentTask = _task;}
    AgvTaskPtr getTask(){return currentTask;}

    int getId(){return id;}
    std::string getName(){return name;}
    std::string getIp(){return ip;}
    int getPort(){return port;}

    void setName(std::string _name){name=_name;}
    void setIp(std::string _ip){ip=_ip;}
    void setPort(int _port){port=_port;}

    //回调
    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();

    void onArriveStation(int station);
    void onLeaveStation(int stationid);
    void onError(int code,std::string msg);
    void onWarning(int code, std::string msg);
    void reconnect();
protected:
    AgvTaskPtr currentTask;
    int id;
    std::string name;
    std::string ip;
    int port;

    virtual void arrve(int x,int y);
    virtual void goStation(int station, bool stop = false);
    virtual void stop();
    virtual void callMapChange(int station);

    QyhTcpClient *tcpClient;

    volatile int arriveStation;

    std::mutex stationMtx;
    std::vector<int> excutestations;
    std::vector<int> excutespaths;

private:
    std::map<std::string,std::string> extra_params;

public:
    void setExtraParam(std::string key,std::string value){extra_params[key] = value;}
    std::string getExtraParam(std::string key){return extra_params[key];}
};

#endif // AGV_H
