
#include <thread>

#include "agv.h"
#include "agvstation.h"
#include "agvline.h"
#include "agvtask.h"
#include "qyhtcpclient.h"


Agv::Agv(int _id, std::string _name, std::string _ip, int _port):
    currentTask(nullptr),
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    lastStation(nullptr),
    nowStation(nullptr),
    nextStation(nullptr),
    tcpClient(nullptr)
{
}

Agv::~Agv()
{
    if(tcpClient)delete tcpClient;
}

void Agv::init(){
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&Agv::onRead,this,std::placeholders::_1,std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&Agv::onConnect,this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&Agv::onDisconnect,this);
    tcpClient = new QyhTcpClient(ip,port,onread,onconnect,ondisconnect);
}

void Agv::onRead(const char *data,int len)
{
    //TODO:
}

void Agv::onConnect()
{
    //TODO
}

void Agv::onDisconnect()
{
    //TODO
}

//到达后是否停下，如果不停下，就是不减速。
void Agv::goStation(AgvStation *station,bool stop)
{
    //发送站点坐标
    //station->x;station->y;
    //阻塞，直到到达这个位置
    //例如:
    //stringstream ss;
    //ss<<"x:"<<x<<",y:"<<y<<".";
    //tcpsocket.write(ss.str().c_str());

}

//请求切换地图(呼叫电梯)
void Agv::callMapChange(AgvStation *station)
{
    if(!station->mapChangeStation)return ;
    //例如:
    //1楼电梯内坐标 (100,100)
    //2楼电梯内坐标 (200,200)
    //3楼电梯内坐标 (300,300)
    //给电梯一个到达1楼并开门的指令
//    if(station->x == 100 && station->y == 100){
//        elevactorGoFloor(1);
//        elevactorOpenDoor();
//    }
//    else if(station->x == 200 && station->y == 200){
//        elevactorGoFloor(2);
//        elevactorOpenDoor();
//    }
//    else if(station->x == 300 && station->y == 300){
//        elevactorGoFloor(3);
//        elevactorOpenDoor();
//    }else{
//        //
//        throw std::exception("地图切换站点错误");
//    }

}

void Agv::excutePath(std::vector<AgvLine *> lines)
{
    std::vector<AgvStation *> stations;

    for(auto line:lines){
        stations.push_back(line->endStation);
    }
    //告诉小车接下来要执行的路径
    AgvStation *next = nullptr;//下一个要去的位置
    for(int i=0;i<stations.size();++i){
        AgvStation *now = stations[i];//接下来要去的位置
        if(i+1<stations.size())
            next = stations[i+1];
        else
            next = nullptr;

        if(next == nullptr){
            //没有下一站了
            goStation(now,true);
            continue;
        }

        //还有下一站。
        //分类讨论
        if(!now->mapChangeStation && !next->mapChangeStation)
        {
            //两个都不是地图切换点
            goStation(now,false);
        }
        else if(!now->mapChangeStation && next->mapChangeStation){
            //下一站是 地图切换点，例如三楼电梯点

            //到达电梯口停下，
            goStation(now,true);

            //并呼叫电梯到达三楼
            callMapChange(next);
        }

        else if(now->mapChangeStation && next->mapChangeStation){
            //下一站是地图切换点，下下站还是，例如下一站是三楼电梯点，而下下站是1楼电梯点
            goStation(now,true);//进电梯
            callMapChange(next);//呼叫到1楼
        }

        else if(now->mapChangeStation && !next->mapChangeStation){
            //下一站是电梯内，但是下下站不是
            goStation(now,true);
            callMapChange(next);//开门
        }


    }

}
