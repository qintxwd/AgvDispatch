#include "virtualrosagv.h"
#include "mapmap/onemap.h"
#include "mapmap/mapmanager.h"

VirtualRosAgv::VirtualRosAgv(int id,std::string name):
    VirtualAgv(id,name)
{

}

VirtualRosAgv::~VirtualRosAgv()
{

}

void VirtualRosAgv::init()
{
    //do nothing
}

void VirtualRosAgv::excutePath(std::vector<int> lines)
{
    //
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

void VirtualRosAgv::cancelTask()
{
    currentTask->cancel();
    //是否保存到数据库呢?
    stop();
}

void VirtualRosAgv::arrve(int x,int y)
{

}

void VirtualRosAgv::goStation(int station, bool stop = false)
{
    //看是否是写特殊点
    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
    if(spirit == nullptr)return ;
    MapPoint *point = static_cast<MapPoint *>(spirit);
    if(point == nullptr)return ;

    //获取当前线路和当前站点
    MapPoint *startPoint = nullptr;
    if(nowStation > 0){
        startPoint = MapManager::getInstance()->getMapSpiritById(nowStation);
    }else{
        startPoint = MapManager::getInstance()->getMapSpiritById(lastStation);
    }

    if(startPoint==nullptr){
        //初始位置未知！
        return ;
    }

    //获取当前走的线路
    MapPath *path = MapManager::getInstance()->getMapPathByStartEnd(startPoint->getId(),station);
    if(path==nullptr){
        //根本不存在该线路
        return ;
    }


    //进行模拟
    while(true){
        //1.向目标前进100ms的距离 假设每次前进10
        if(path->getPathType() == MapPath::Map_Path_Type_Line){
            //当前位置向 目标点移动10

        }else if(path->getPathType() == MapPath::Map_Path_Type_Quadratic_Bezier){

        }else if(path->getPathType() == MapPath::Map_Path_Type_Cubic_Bezier){

        }
        //2.初次移动，调用离开上一站

        //3.重新计算当前位置

        //4.判断是否到达下一站 如果到达，break

        Sleep(100);
    }
}

void VirtualRosAgv::stop()
{
    isStop = true;
}

void VirtualRosAgv::callMapChange(int station)
{
    //模拟电梯运行，这里只做等待即可
    Sleep(15000);
}
