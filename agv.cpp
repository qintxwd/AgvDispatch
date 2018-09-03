
#include <thread>

#include "agv.h"
#include "mapmap/mapmanager.h"
#include "agvtask.h"
#include "network/tcpclient.h"
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

void Agv::setPosition(int _lastStation, int _nowStation, int _nextStation) {
    lastStation = _lastStation;
    nowStation = _nowStation;
    nextStation = _nextStation;

    if (nowStation > 0) {
        onArriveStation(nowStation);
    }
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
    auto mapmanagerptr = MapManager::getInstance();
    int block = -1;

    //add block occur station
    block = mapmanagerptr->getBlock(station);
    if(block!=-1)
        mapmanagerptr->addBlcokOccu(block,getId(),station);

    //free block last path and station
    MapPoint *point = mapmanagerptr->getPointById(station);
    if(point == nullptr)return ;
    combined_logger->info("agv id:{0} arrive station:{1}",getId(),point->getName());

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
    stationMtx.lock();
    for(auto line:excutespaths){
        MapPath *path = mapmanagerptr->getPathById(line);
        if(path == nullptr)continue;
        paths.push_back(path);
    }
    stationMtx.unlock();

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
        for(int i=0;i<=findIndex;++i){
            auto line = paths[i];
            int start = line->getStart();
            int end = line->getEnd();
            int lineId = line->getId();
            //free start station
            mapmanagerptr->freeStation(start,shared_from_this());
            block = mapmanagerptr->getBlock(start);
            if(block!=-1)
                mapmanagerptr->addBlcokOccu(block,getId(),station);

            //free end station
            if(i!=findIndex){
                mapmanagerptr->freeStation(end,shared_from_this());
                block = mapmanagerptr->getBlock(end);
                if(block!=-1)
                    mapmanagerptr->addBlcokOccu(block,getId(),end);
            }

            //free last line
            mapmanagerptr->freeLine(lineId,shared_from_this());
            block = mapmanagerptr->getBlock(lineId);
            if(block!=-1)
                mapmanagerptr->addBlcokOccu(block,getId(),lineId);
        }
    }

    char buf[SQL_MAX_LENGTH];
    snprintf(buf, SQL_MAX_LENGTH, "update agv_agv set lastStation=%d,nowStation=%d,nextStation=%d  where id = %d;", id, lastStation, nowStation, nextStation);
    try {
        g_db.execDML(buf);
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0} ", e.what());
    }

    //判断下一段线路的可行性 [block]
    //判断下一个线路 是否在block内，block内是否已经有其他车辆。如果有的话，就暂停 一下当前动作
    stationMtx.lock();
    auto itr = std::find(excutestations.begin(),excutestations.end(),nowStation);
    if(itr!=excutestations.end()){
        ++itr;
        if(itr!=excutestations.end()){
            nextStation = *itr;
            stationMtx.unlock();
            //next path is block free
            auto p = mapmanagerptr->getPathByStartEnd(nowStation,nextStation);
            if(p!=nullptr){
                auto b = mapmanagerptr->getBlock(p->getId());
                if(b!=-1){
                    if(!mapmanagerptr->blockPassable(b,getId())){
                        pause();
                    }
                }
            }
        }else{
            stationMtx.unlock();
        }
    }else{
        stationMtx.unlock();
    }
}

void Agv::onLeaveStation(int stationid)
{
    auto mapmanagerptr = MapManager::getInstance();
    //add block occur station
    auto block = mapmanagerptr->getBlock(stationid);
    if(block!=-1)
        mapmanagerptr->freeBlcokOccu(block,getId(),stationid);

    //free block last path
    auto lastpath = mapmanagerptr->getPathByStartEnd(stationid,nextStation);
    if(lastpath!=nullptr){
        block = mapmanagerptr->getBlock(lastpath->getId());
        if(block!=-1){
            mapmanagerptr->addBlcokOccu(block,getId(),lastpath->getId());
        }
    }

    nowStation = 0;
    lastStation = stationid;

    auto point = mapmanagerptr->getPointById(stationid);
    if(point!=nullptr){
        combined_logger->info("agv id:{0} leave station:{1}",getId(),point->getName());
    }
    //add block occur
    auto line = mapmanagerptr->getPathByStartEnd(lastStation,nextStation);
    if(line!=nullptr){
        auto block = mapmanagerptr->getBlock(line->getId());
        if(block!=-1){
            mapmanagerptr->addBlcokOccu(block,getId(),line->getId());
        }
    }

    //释放这个站点的占用
    MapManager::getInstance()->freeStation(stationid,shared_from_this());

    bool b_getNextStation = false;
    stationMtx.lock();
    for(auto line:excutespaths){
        MapPath *path = mapmanagerptr->getPathById(line);
        if(path == nullptr)continue;
        if(path->getStart() == nowStation){
            nextStation = path->getEnd();
            b_getNextStation = true;
            break;
        }
    }
    stationMtx.unlock();
    if(!b_getNextStation){
        nextStation = 0;
    }

    char buf[SQL_MAX_LENGTH];
    snprintf(buf, SQL_MAX_LENGTH, "update agv_agv set lastStation=%d,nowStation=%d,nextStation=%d  where id = %d;", id, lastStation, nowStation, nextStation);
    try {
        g_db.execDML(buf);
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0} ", e.what());
    }

    //判断下一个站点的可行性 [block]
    //判断下一个站点 是否在block内，block内是否已经有其他车辆。如果有的话，就暂停 一下当前动作
    auto p = MapManager::getInstance()->getPointById(nextStation);
    if(p!=nullptr){
        auto b = MapManager::getInstance()->getBlock(p->getId());
        if(b!=-1){
            if(!MapManager::getInstance()->blockPassable(b,getId())){
                pause();
            }
        }
    }
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

