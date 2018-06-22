
#include <thread>

#include "agv.h"
#include "mapmap/mapmanager.h"
#include "agvtask.h"
#include "qyhtcpclient.h"
#include "userlogmanager.h"
#include "msgprocess.h"
#include "mapmap/mapmanager.h"


Agv::Agv(int _id, std::string _name, std::string _ip, int _port,int _agvType, int _agvClass, std::string _lineName) :
    currentTask(nullptr),
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    lastStation(0),
    nowStation(0),
    nextStation(0),
    tcpClient(nullptr),
    arriveStation(0),
    agvType(_agvType),
    agvClass(_agvClass),
    lineName(_lineName)
{
}

Agv::~Agv()
{
    if (tcpClient)delete tcpClient;
}

void Agv::init() {
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&Agv::onRead, this, std::placeholders::_1, std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&Agv::onConnect, this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&Agv::onDisconnect, this);
    tcpClient = new QyhTcpClient(ip, port, onread, onconnect, ondisconnect);
}

void Agv::onConnect()
{
    //TODO
}

void Agv::onDisconnect()
{
    //TODO
}
void Agv::reconnect()
{
    tcpClient->resetConnect(ip, port);
}
//到达后是否停下，如果不停下，就是不减速。
//是一个阻塞的函数
void Agv::goStation(int station, bool stop)
{

    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
    if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return;

    //TODO:获取block信息//如果该站点属于一个block。判断该block是否已经有车辆，....
    MapPoint *point = static_cast<MapPoint *>(spirit);

    point->getId();
    point->getRealX();
    point->getRealY();
}

void Agv::stop()
{

}

void Agv::arrve(int x, int y) {

    //for(auto station:excutestations){
    //    if(sqrt(pow(x-station->getX(),2)+pow(x-station->getX(),2))<20){
    //        onArriveStation(station);
    //        break;
    //    }
    //}
}

void Agv::onArriveStation(int station)
{
    if(station>0){
        arriveStation = station;
        if(nowStation>0){
            lastStation = nowStation;
        }
        nowStation = station;
        stationMtx.lock();
        for (int i = 0; i < excutestations.size(); ++i) {
            if (excutestations[i] == station) {
                if (i + 1 < excutestations.size()) {
                    nextStation = excutestations[i + 1];
                }else{
                    nextStation = 0;
                }
                break;
            }
        }
        stationMtx.unlock();
    }
    //TODO:释放之前的线路和站点
    std::vector<MapPath *> paths;
    for(auto line:excutespaths){
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if(spirit ==nullptr)continue;
        if(spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;
        MapPath *path = static_cast<MapPath *>(spirit);
        paths.push_back(path);
    }

    int findIndex = -1;
    for(int i=0;i<paths.size();++i){
        auto line = paths[i];
        if(line->getEnd() == station){
            findIndex = i;
        }
    }
    if(findIndex != -1){
        //之前的道路占用全部释放
        //之前的站点占用全部释放
        for(int i=0;i<findIndex;++i){
            auto line = paths[i];
            int end = line->getEnd();
            int lineId = line->getId();
            MapManager::getInstance()->freeStation(end,shared_from_this());
            MapManager::getInstance()->freeLine(lineId,shared_from_this());
        }

    }
}

void Agv::onLeaveStation(int stationid)
{
    if (nowStation == stationid) {
        nowStation = 0;
        lastStation = stationid;
    }
    //释放这个站点的占用
    MapManager::getInstance()->freeStation(stationid,shared_from_this());
}

void Agv::onError(int code, std::string msg)
{
    status = AGV_STATUS_ERROR;
    char sss[1024];
    snprintf(sss, 1024, "Agv id:%d occur error code:%d msg:%s", id, code, msg.c_str());
    std::string ss(sss);
    //combined_logger->error(ss.c_str());
    UserLogManager::getInstance()->push(ss);
    MsgProcess::getInstance()->errorOccur(code, msg, true);
}

void Agv::onWarning(int code, std::string msg)
{
    char sss[1024];
    snprintf(sss, 1024, "Agv id:%d occur warning code:%d msg:%s", id, code, msg.c_str());
    std::string ss(sss);
    //combined_logger->warn(ss.c_str());
    UserLogManager::getInstance()->push(ss);
    MsgProcess::getInstance()->errorOccur(code, msg, false);
}

//请求切换地图(呼叫电梯)
void Agv::callMapChange(int station)
{
    //if(!station->getMapChange())return ;
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

void Agv::excutePath(std::vector<int> lines)
{
    excutespaths = lines;
    stationMtx.lock();
    excutestations.clear();
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int endId = path->getEnd();
        excutestations.push_back(endId);

        ////获取站点信息方法：
        //MapSpirit *spirit2 = MapManager::getInstance()->getMapSpiritById(endId);
        //if (spirit2 == nullptr || spirit2->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;

        //MapPoint *point = static_cast<MapPoint *>(spirit2);
        //point->getId();
        //point->getRealX();
        //point->getName();
        //point->getRealY();
    }
    stationMtx.unlock();

    int next = 0;//下一个要去的位置
    for(int i=0;i<excutestations.size();++i)
    {
        if(currentTask!=nullptr && currentTask->getIsCancel())break;//任务取消
        if(status == AGV_STATUS_HANDING)break;//手动控制
        if(status == AGV_STATUS_ERROR)break;//发生错误

        int now = excutestations[i];//接下来要去的位置
        if(i+1<excutestations.size())
            next = excutestations[i+1];
        else
            next = 0;

        if(next == 0){
            //没有下一站了
            goStation(now,true);
            continue;
        }

        MapSpirit *spirit1 = MapManager::getInstance()->getMapSpiritById(now);
        MapSpirit *spirit2 = MapManager::getInstance()->getMapSpiritById(next);
        if (spirit2 == nullptr || spirit2->getSpiritType() != MapSpirit::Map_Sprite_Type_Point) {
            goStation(now, true);
            continue;
        }

        MapPoint *nowPoint = static_cast<MapPoint *>(spirit1);
        MapPoint *nextPoint = static_cast<MapPoint *>(spirit2);

        //还有下一站。
        //分类讨论
        if(!nextPoint->getMapChange() && !nextPoint->getMapChange())
        {
            //两个都不是地图切换点
            goStation(now,false);
        }
        else if(!nowPoint->getMapChange() && nextPoint->getMapChange()){
            //下一站是 地图切换点，例如三楼电梯点

            //到达电梯口停下，
            goStation(now,true);
            if(currentTask!=nullptr && currentTask->getIsCancel())break;//任务取消
            if(status == AGV_STATUS_HANDING)break;//手动控制
            if(status == AGV_STATUS_ERROR)break;//发生错误
            //并呼叫电梯到达三楼
            callMapChange(next);
        }

        else if(nowPoint->getMapChange() && nextPoint->getMapChange()){
            //下一站是地图切换点，下下站还是，例如下一站是三楼电梯点，而下下站是1楼电梯点
            goStation(now,true);//进电梯
            callMapChange(next);//呼叫到1楼
        }

        else if(nowPoint->getMapChange() && !nextPoint->getMapChange()){
            //下一站是电梯内，但是下下站不是
            goStation(now,true);
            callMapChange(next);//开门
        }
    }
    if(currentTask!=nullptr && currentTask->getIsCancel())cancelTask();
    if(status == AGV_STATUS_HANDING || status == AGV_STATUS_ERROR)cancelTask();
}

void Agv::cancelTask()
{
}

void Agv::onRead(const char *data, int len)
{

}

bool Agv::send(const char *data, int len)
{
    return tcpClient->sendToServer(data, len);
}
