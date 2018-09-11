#ifndef ELEVATOR_H
#define ELEVATOR_H
#include <memory>
#include <mutex>
#include <map>
#include "device/device.h"
#include "elevator_protocol.h"

class Elevator;
using ElevatorPtr = std::shared_ptr<Elevator>;

class ElevatorPositon
{
private:
    std::string _name;
    bool _isOccupied;
    bool _isEnabled;
    int _agvId;

public:
    ElevatorPositon(std::string name,int agvId, bool isOccupied,bool isEnabled)
    {
        _name=name;
        _isOccupied=isOccupied;
        _isEnabled=isEnabled;
        _agvId=agvId;
    }

    ~ElevatorPositon(){}

    std::string getName()
    {
        return _name;
    }

    void setOccupied(bool isOccupied)
    {
        _isOccupied=isOccupied;
    }
    bool isOccupied()
    {
        return _isOccupied;
    }

    void setEnabled(bool isEnabled)
    {
        _isEnabled=isEnabled;
    }
    bool isEnabled()
    {
        return _isEnabled;
    }

    void setAGVId(int agvId)
    {
        _agvId=agvId;
    }


    int getAGVId()
    {
        return _agvId;
    }

};


//DEVICE
class Elevator : public Device
{
public:
    using EleParam = lynx::elevator::Param;

    Elevator(int id,std::string name,std::string ip,int port, bool leftEnabled, bool rightEnabled, bool elevatorEnabled,std::string _waitingPoints);
    virtual ~Elevator();

    static int POSITION_NO_AGV;

    static std::string POSITION_LEFT;
    static std::string POSITION_RIGHT;
    static std::string POSITION_MIDDLE; //乘坐电梯最大数为1时使用

    // 一些测试用函数
    // 测试状态切换
    void SwitchElevatorState(int elevator_id);
    // 复位电梯状态
    bool ResetElevatorState(int elevator_id);
    // 电梯状态询问
    std::shared_ptr<EleParam> GetElevatorState(int elevator_id);
    // 内部通信测试
    bool PingElevator(int elevator_id);



    /*请求乘电梯
     *from_floor: AGV 所在楼层
     *to_floor: 到达楼层
     *elevator_id: 电梯ID
     *agv_id: AGV ID
     *返回值 此函数会阻塞 直到返回请求, true 为接受请求门打开, false为拒绝
     *timeout: 秒, 超时返回
     * 返回值: 可以乘坐电梯编号, -1表示没有请求成功
     */
    virtual int RequestTakeElevator(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    // (乘梯应答) agv进入时需要定时调用
    void TakeEleAck(int from_floor, int to_floor, int elevator_id, int agv_id);

    // 确认需要乘坐电梯(乘梯应答), 进入电梯过程中会持续发送(乘梯应答)
    // 返回值: true 表示内呼回应(进入指令), agv可以乘坐, 并且agv验证信息通过
    // 此函数阻塞, 若返回值为true, 此时agv可进入电梯.
    bool ConfirmEleInfo(int from_floor, int to_floor,
                        int elevator_id, int agv_id, int timeout);

    // (进入电梯应答) agv进入后, 直到可以离开
    bool AgvEnterUntilArrive(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    bool AgvStartLeft(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    // agv完全离开电梯, 结束流程
    bool AgvLeft(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    // (进入电梯应答)
    bool AgvEnterConfirm(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    // wait agv离开电梯
    bool AgvWaitArrive(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);

    void StartSendThread(int cmd, int from_floor, int to_floor, int elevator_id, int agv_id);
    void StopSendThread();


    virtual bool OpenDoor(int floor);
    virtual void CloseDoor(int floor);

    //保持电梯门打开
    virtual void KeepingDoorOpen(int floor);
    inline bool IsConnected() const { return connected_; }

    bool IsHasFreePosition()
    {
        bool result = false;
        std::map<std::string,ElevatorPositon *>::iterator iter;//定义一个迭代指针iter

        elevatorMutex.lock();
        for(iter=position_list_.begin(); iter!=position_list_.end(); iter++)
        {
            if(iter->second != nullptr)
            {
                ElevatorPositon * p = iter->second;
                if(p->isEnabled() && !p->isOccupied())
                {
                    result = true;
                    break;
                }
            }
        }
        elevatorMutex.unlock();
        return result;
    }

    std::string RequestFreePositon()
    {
        std::string result = "";

        std::map<std::string,ElevatorPositon *>::iterator iter;//定义一个迭代指针iter

        elevatorMutex.lock();
        for(iter=position_list_.begin(); iter!=position_list_.end(); iter++)
        {
            if(iter->second != nullptr)
            {
                ElevatorPositon * p = iter->second;
                if(p->isEnabled() && !p->isOccupied())
                {
                    p->setOccupied(true);
                    result = p->getName();
                    break;
                }
            }
        }
        elevatorMutex.unlock();
        return result;
    }

    //AGV 已经进入电梯,
    void AgvIn(int agv_id, std::string position)
    {
        elevatorMutex.lock();

        if(agv_num < MAX_AGV_NUM)
        {
            ElevatorPositon * p = position_list_[position];
            if(p != nullptr)
            {
                p->setOccupied(true);
                p->setAGVId(agv_id);

                agv_num++;
            }
        }

        elevatorMutex.unlock();
    }

    //AGV 已经出电梯
    void AgvOut(int agv_id, std::string position)
    {
        elevatorMutex.lock();

        if(agv_num > 0)
        {
            ElevatorPositon * p = position_list_[position];
            if(p != nullptr)
            {
                p->setOccupied(false);
                p->setAGVId(POSITION_NO_AGV);

                agv_num--;
            }
        }

        elevatorMutex.unlock();
    }

private:
    // 通知不必有信息返回
    bool notify(const EleParam& p);
    // 请求, 有响应时返回响应内容, 超时返回nullptr
    std::shared_ptr<EleParam> request(const EleParam& p, int cmd, int timeout);
    // 响应, 等待服务器指令
    std::shared_ptr<EleParam> waitfor(int agv_id, lynx::elevator::CMD cmd, int timeout);
    //回调
    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();

    virtual void reconnect();

    //电梯到达楼层
    virtual void onElevatorArrived(int floor);

    int agv_num;
    static int MAX_AGV_NUM; //AGV 乘坐电梯最大数

    std::mutex elevatorMutex;

    std::map<std::string,ElevatorPositon *> position_list_; //电梯位置状态
    //std::map<std::string,int> position_status_list_;
    //

    // connected
    std::atomic_bool       connected_;
    // request contex
    struct ElevatorCtx;
    std::unique_ptr<ElevatorCtx> ctx_;

    bool left_pos_enabled;  //启用电梯左侧位置
    bool right_pos_enabled; //启用电梯右侧位置
    bool elevator_enabled;  //启用电梯
    std::vector<int> waitingPoints; //电梯旁边等待区point

    bool send_cmd;  //启用电梯
    bool resetFlag;
};



#endif // ELEVATOR_H
