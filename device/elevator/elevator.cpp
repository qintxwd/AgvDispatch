#include "elevator.h"

Elevator::Elevator(int _id, std::string _name, std::string _ip, int _port)
    : Device(_id, _name, _ip,  _port)
{
    agv_num = 0;

    if(MAX_AGV_NUM == 1)
    {
        ElevatorPositon* p = new ElevatorPositon(POSITION_MIDDLE, POSITION_NO_AGV, false, true);
        position_list_[POSITION_MIDDLE] = p;

        position_list_[POSITION_LEFT] = nullptr;
        position_list_[POSITION_RIGHT] = nullptr;
    }
    else if(MAX_AGV_NUM == 2)
    {
        ElevatorPositon* p_left = new ElevatorPositon(POSITION_LEFT, POSITION_NO_AGV, false, true);
        position_list_[POSITION_LEFT] = p_left;

        ElevatorPositon* p_right = new ElevatorPositon(POSITION_RIGHT, POSITION_NO_AGV, false, true);
        position_list_[POSITION_RIGHT] = p_right;

        position_list_[POSITION_MIDDLE] = nullptr;
    }
}

Elevator::~Elevator()
{
    std::map<std::string,ElevatorPositon *>::iterator iter;//定义一个迭代指针iter

    for(iter=position_list_.begin(); iter!=position_list_.end(); iter++)
    {
        if(iter->second != nullptr)
            delete iter->second;
    }

}

/*请求乘电梯
 *from_floor: AGV 所在楼层
 *to_floor: 到达楼层
 *elevator_id: 电梯ID
 *agv_id: AGV ID
 *返回值 此函数会阻塞 直到返回请求, true 为接受请求门打开, false为拒绝
 *timeout: 秒, 超时返回
 */
bool Elevator::RequestTakeElevator(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    return false;
}

bool Elevator::OpenDoor(int floor)
{
    return false;
}

void Elevator::CloseDoor(int floor)
{

}

//保持电梯门打开
void Elevator::KeepingDoorOpen(int floor)
{

}

//电梯到达楼层
void Elevator::onElevatorArrived(int floor)
{

}


void Elevator::onRead(const char *data,int len)
{

}

void Elevator::onConnect()
{

}

void Elevator::onDisconnect()
{

}

void Elevator::reconnect()
{

}
