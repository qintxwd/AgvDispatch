#ifndef ELEVATOR_H
#define ELEVATOR_H
#include <memory>
#include <mutex>
#include <map>
#include "device/device.h"

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
    Elevator(int id,std::string name,std::string ip,int port);
    virtual ~Elevator();

    static int POSITION_NO_AGV;

    static std::string POSITION_LEFT;
    static std::string POSITION_RIGHT;
    static std::string POSITION_MIDDLE; //乘坐电梯最大数为1时使用

    /*请求乘电梯
     *from_floor: AGV 所在楼层
     *to_floor: 到达楼层
     *elevator_id: 电梯ID
     *agv_id: AGV ID
     *返回值 此函数会阻塞 直到返回请求, true 为接受请求门打开, false为拒绝
     *timeout: 秒, 超时返回
     */
    virtual bool RequestTakeElevator(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout);
    virtual bool OpenDoor(int floor);
    virtual void CloseDoor(int floor);

    //保持电梯门打开
    virtual void KeepingDoorOpen(int floor);

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
};

int Elevator::POSITION_NO_AGV = -1;
std::string Elevator::POSITION_LEFT = "left";
std::string Elevator::POSITION_RIGHT = "right";
std::string Elevator::POSITION_MIDDLE = "middle";
int Elevator::MAX_AGV_NUM=2; //AGV 乘坐电梯最大数


#endif // ELEVATOR_H
