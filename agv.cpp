
#include <thread>

#include "agv.h"
#include "mapmap/mapmanager.h"
#include "agvtask.h"
#include "qyhtcpclient.h"
#include "userlogmanager.h"
#include "msgprocess.h"
#include "mapmap/mapmanager.h"


Agv::Agv(int _id, std::string _name, std::string _ip, int _port) :
    currentTask(nullptr),
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    lastStation(0),
    nowStation(0),
    nextStation(0),
    x(0),
    y(0),
    theta(0)
{
}

Agv::~Agv()
{
}

void Agv::init()
{
}

//到达后是否停下，如果不停下，就是不减速。
//是一个阻塞的函数
void Agv::goStation(int station, bool stop)
{
}

void Agv::stop()
{
}

void Agv::onArriveStation(int station)
{

    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);

    if(spirit->getSpiritType()!=MapSpirit::Map_Sprite_Type_Point)return ;

    MapPoint *point = static_cast<MapPoint *>(spirit);

    x = point->getX();
    y = point->getY();

    if(station>0){
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
}

void Agv::cancelTask()
{
}

void Agv::reconnect()
{
}

