#ifndef AGV_H
#define AGV_H
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>

//3层升降货架是否有料信息KEY
#define ALL_FLOOR_INFO_KEY         "all_floor_info"
//AGV完成一个Task是否需要回到等待区KEY
#define NEED_AGV_BACK_TO_WAITING_AREA_KEY  "need_agv_back_to_waiting_area"

class AgvTask;
using AgvTaskPtr = std::shared_ptr<AgvTask>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;
//AGV
class Agv:public std::enable_shared_from_this<Agv>
{
public:
    Agv(int id,std::string name,std::string ip,int port);

    virtual void init();

	virtual void setPosition(int _lastStation, int _nowStation, int _nextStation);

    virtual ~Agv();

    enum { Type = 0 };

    virtual int type(){return Type;}

    //阻塞的函数,知道执行完成后才可以返回
    virtual void excutePath(std::vector<int> lines) = 0;

    virtual void cancelTask();

    virtual void reconnect();

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
        AGV_STATUS_NOTREADY = 7 //刚连接，尚未上报位置
    };

    //状态
    enum {
        AGV_TYPE_THREE_UP_DOWN_LAYER_SHELF = 0,//3层升降货架AGV,  群创
    };

    int status = AGV_STATUS_IDLE;

    void setTask(AgvTaskPtr _task){currentTask = _task;}
    AgvTaskPtr getTask(){return currentTask;}
    int getId(){return id;}
    std::string getName(){return name;}
    std::string getIp(){return ip;}
    int getPort(){return port;}
    void setName(std::string _name){name=_name;}
    void setIp(std::string _ip){ip=_ip;}
    void setPort(int _port){port=_port;}
    void setX(int _x){x=_x;}
    void setY(int _y){y=_y;}
    void setTheta(int _theta){theta=_theta;}
    int getX(){return x;}
    int getY(){return y;}
    int getTheta(){return theta;}
	int getLastStation() { return lastStation; }
	int getNowStation() { return nowStation; }
	int getNextStation() { return nextStation; }

    void onArriveStation(int station);
    void onLeaveStation(int stationid);
    void onError(int code,std::string msg);
    void onWarning(int code, std::string msg);

    virtual void onTaskStart(AgvTaskPtr _task){}
    virtual void onTaskFinished(AgvTaskPtr _task){}

    virtual void goStation(int station, bool stop = false);
    virtual void stop();
    virtual void callMapChange(int station);

    virtual bool pause(){return true;}
    virtual bool resume(){return true;}

    void setExtraParam(std::string key,std::string value){extra_params[key] = value;}
    std::string getExtraParam(std::string key){return extra_params[key];}
protected:
    AgvTaskPtr currentTask;
    int id;
    std::string name;
    std::string ip;
    int port;

    //位置信息//直接显示要显示的位置
    double x;
    double y;
    double theta;

	//计算路径用的
	int lastStation;//上一个站点
	int nowStation;//当前所在站点
	int nextStation;//下一个站点

    std::mutex stationMtx;
    std::vector<int> excutestations;
    std::vector<int> excutespaths;

    std::map<std::string,std::string> extra_params;
};

#endif // AGV_H
