#include "dyforklift.h"
#include "../common.h"
#include "../mapmap/mappoint.h"
#include "../device/elevator/elevator.h"
#include "charge/chargemachine.h"
#include <QByteArray>
#include <QString>
//#define RESEND
//#define HEART

DyForklift::DyForklift(int id, std::string name, std::string ip, int port):
    Agv(id,name,ip,port)
{
    startpoint = -1;
    actionpoint = -1;
    status = Agv::AGV_STATUS_NOTREADY;
    init();
    //    充电机测试代码
    //    unsigned char charge_id = 1;
    //    chargemachine cm(charge_id, "charge_machine", "127.0.0.1", 5566);
    //    cm.init();
    //    cm.chargeControl(charge_id, CHARGE_START);
    //    cm.chargeQuery(charge_id, 0x01);
}

void DyForklift::init(){

#ifdef RESEND
    g_threadPool.enqueue([&, this] {
        while (true) {
            std::map<int, DyMsg >::iterator iter;
            if(msgMtx.try_lock())
            {
                for(iter = this->m_unRecvSend.begin(); iter != this->m_unRecvSend.end(); iter++)
                {
                    iter->second.waitTime++;
                    combined_logger->info("lastsend:{0}, waitTime:{1}", iter->first, iter->second.waitTime);
                    //TODO waitTime>n resend
                    if(iter->second.waitTime > MAX_WAITTIMES)
                    {
                        char* temp = new char[iter->second.msg.length()+1];
                        strcpy(temp, iter->second.msg.data());
                        this->m_qTcp->doSend(temp, iter->second.msg.length());
                        delete temp;
                        iter->second.waitTime = 0;
                    }
                }
                msgMtx.unlock();
            }
            sleep(6);

        }
    });
#endif
#ifdef HEART
    g_threadPool.enqueue([&, this] {
        while (true) {
            heart();
            usleep(500000);

        }
    });
#endif
}

void DyForklift::onTaskStart(AgvTaskPtr _task)
{
    if(_task != nullptr)
    {
        status = Agv::AGV_STATUS_TASKING;
    }
}

void DyForklift::onTaskFinished(AgvTaskPtr _task)
{

}

bool DyForklift::resend(const char *data,int len){
    if(!data||len<=0)return false;
    bool sendResult = send(data,len);
    //    bool sendResult = true;

    int resendTimes = 0;
    while(!sendResult && ++resendTimes< maxResendTime){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(500));
        sendResult = send(data,len);
    }
    return sendResult;
}

bool DyForklift::turn(float speed, float angle)
{
    std::stringstream body;
    body<<FORKLIFT_TURN<<speed<<"|"<<angle;
    return resend(body.str().c_str(), body.str().length());
}

bool DyForklift::fork(int params)
{
    m_lift = (params!= 0) ? true: false;
    std::stringstream body;
    body<<FORKLIFT_FORK;
    body.width(2);
    body.fill('0');
    body<<params;
    return resend(body.str().c_str(), body.str().length());
}

bool DyForklift::heart()
{
    std::stringstream body;
    body.fill('0');
    body.width(2);
    body<<FORKLIFT_HEART;
    body<<m_lift<<m_lift;
    return resend(body.str().c_str(), body.str().length());
}
bool DyForklift::waitTurnEnd(int waitMs)
{
    while(!isFinish(FORKLIFT_TURN) && --waitMs>0){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(1));
    }
    return turnResult;
}

Pose4D DyForklift::getPos()
{
    return m_currentPos;
}
int DyForklift::nearestStation(int x, int y, int floor)
{
    int minDis = -1;
    int min_station = -1;
    std::vector<int> stations = MapManager::getInstance()->getStations(floor);
    for (auto station : stations) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;
        MapPoint *point = static_cast<MapPoint *>(spirit);
        long dis = pow(x-point->getRealX(),2)+pow(y-point->getRealY(),2);
        if(min_station == -1 || minDis > dis)
        {
            minDis = dis;
            min_station = point->getId();
        }
    }
    return min_station;
}

void DyForklift::onRecv(const char *data,int len)
{
    combined_logger->info("agv{0} recv data:{1}", id, data);
}
//解析小车上报消息
void DyForklift::onRead(const char *data,int len)
{

    if(data == NULL || len <= 0)return ;
    //#ifdef QYHTCP
    //    std::string msg(data+1,len-2);
    //    int length = std::stoi(msg.substr(6, 4));
    //    if(length != len-2)
    //    {
    //        return;
    //    }
    //#else
    std::string msg(data,len);
    int length = std::stoi(msg.substr(6, 4));
    if(length != len)
    {
        return;
    }
    //#endif

    int mainMsg = std::stoi(msg.substr(10, 2));
    std::string body = msg.substr(12);

    if(FORKLIFT_POS != mainMsg)
    {
        combined_logger->info("agv{0} recv data:{1}", id, data);
    }
    //解析小车发送的消息
    switch (mainMsg) {
    case FORKLIFT_POS:
        //小车上报的位置信息
    {
        //*1234560031290|0|0|0.1,0.2,0.3,4#
        std::vector<std::string> all = split(body, "|");
        if(all.size() == 4)
        {
            //status = std::stoi(all[0]);
            //任务线程需要根据此状态判断小车是否在执行任务，不能赋值
            std::vector<std::string> temp = split(all[3], ",");
            if(temp.size() == 4)
            {
                m_currentPos = Pose4D(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]), std::stoi(temp[3]));

                x = m_currentPos.m_x*100;
                y = -m_currentPos.m_y*100;
                theta = -m_currentPos.m_theta*57.3;
                if(AGV_STATUS_NOTREADY == status)
                {
                    //find nearest station
                    nowStation = nearestStation(m_currentPos.m_x*100, -m_currentPos.m_y*100, m_currentPos.m_floor);
                    if(nowStation != -1)
                        status = AGV_STATUS_IDLE;
                }
                else
                {
                    arrve(m_currentPos.m_x*100, -m_currentPos.m_y*100);
                }

            }
        }

        break;
    }
    case FORKLIFT_BATTERY:
    {
        //小车上报的电量信息
        m_power = std::stoi(body);
        break;
    }
    case FORKLIFT_FINISH:
    {
        //小车上报运动结束状态或自定义任务状态
        if(1 == std::stoi(body.substr(2)))
        {
            //command finish
            std::map<int, DyMsg>::iterator iter = m_unFinishCmd.find(std::stoi(body.substr(0,2)));
            if(iter != m_unFinishCmd.end())
            {
                m_unFinishCmd.erase(iter);
            }
        }
        break;
    }
    case FORKLIFT_MOVE:
    case FORKLIFT_TURN:
    case FORKLIFT_FORK:
    case FORKLIFT_STARTREPORT:
    {
        msgMtx.lock();
        //command response
        std::map<int, DyMsg>::iterator iter = m_unRecvSend.find(std::stoi(msg.substr(0,6)));
        if(iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        msgMtx.unlock();
        break;
    }
    default:
        break;
    }
}
//判断小车是否到达某一站点
void DyForklift::arrve(int x, int y) {

    int visitflag = true;

    for(auto station:excutestations){
        if(visitflag)
        {
            if(station != this->nowStation)
            {
                //startlift
                if(!startLift)
                {
                    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(startpoint);
                    if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;

                    MapPoint *point = static_cast<MapPoint *>(spirit);
                    if(func_dis(x, y, point->getRealX(), point->getRealY())>START_RANGE){
                        startLift = true;
                        //                    m_lift = true;
                        fork(FORKLIFT_UP);
                    }
                }
                continue;
            }
            else
            {
                visitflag = false;
            }
        }
        else
        {
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;

            MapPoint *point = static_cast<MapPoint *>(spirit);

            if(func_dis(x, y, point->getRealX(), point->getRealY())<PRECISION && station != this->nowStation){
                combined_logger->info("agv{0} arrive station:{1}", id, point->getName());
                onArriveStation(station);
                this->lastStation = this->nowStation;
                this->nowStation = station;
            }
        }
    }
    //pick
    if(TASK_PICK == task_type && !actionFlag)
    {
        //std::vector<int> stations = MapManager::getInstance()->getStations(m_currentPos.m_floor);
        //int samefloor = std::count(stations.begin(), stations.end(), actionpoint);
        //if(samefloor)
        bool sameFloor = MapManager::getInstance()->isSameFloor(m_currentPos.m_floor, actionpoint);
        if(sameFloor)
        {
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(actionpoint);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return;

            MapPoint *point = static_cast<MapPoint *>(spirit);
            if(func_dis(x, y, point->getRealX(),point->getRealY())<PRECISION){
                actionFlag = true;
                startLift = true;
                fork(FORKLIFT_DOWN);
                //                        m_lift = false;
                return;
            }
        }
    }

    //startlift
    if(!startLift)
    {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(startpoint);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return;

        MapPoint *point = static_cast<MapPoint *>(spirit);
        if(func_dis(x, y, point->getRealX(), point->getRealY())>START_RANGE){
            startLift = true;
            //                    m_lift = true;
            fork(FORKLIFT_UP);
        }
    }
}
//进电梯时用
bool DyForklift::move(float speed, float distance)
{
    std::stringstream body;
    body<<FORKLIFT_MOVE_NOLASER<<speed<<"|"<<distance;
    return resend(body.str().c_str(), body.str().length());
}

//执行路径规划结果
void DyForklift::excutePath(std::vector<int> lines)
{
    combined_logger->info("dyForklift{0} excutePath", id);
    for(auto line:lines)
    {
        std::cout<<line<<" ";
    }
    std::cout<<std::endl;
    stationMtx.lock();
    excutestations.clear();
    excutestations.push_back(nowStation);
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int endId = path->getEnd();
        excutestations.push_back(endId);
        excutespaths.push_back(line);
    }
    stationMtx.unlock();

    actionpoint = 0;
    MapPoint *startstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.front()));
    MapPoint *endstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.back()));
    startpoint = startstation->getId();
    startLift = false;

    AgvTaskPtr currentTask =this->getTask();

    task_type = currentTask->getTaskNodes().at(currentTask->getDoingIndex())->getType();
    combined_logger->info("taskType: {0}", task_type);

    switch (task_type) {
    case TASK_PICK:
    {
        //TODO
        //多层楼需要修改

        if(func_dis(startstation->getX(), startstation->getY(), endstation->getX(), endstation->getY()) < START_RANGE*2)
        {
            startLift = true;
        }
        actionpoint = startstation->getId();

        double dis = 0;
        for (int i = lines.size()-1; i >= 0; i--) {
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(-lines.at(i));
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_DyPath)continue;

            DyMapPath *path = static_cast<DyMapPath *>(spirit);
            dis += func_dis(path->getP2x(), path->getP2y(), path->getP1x(), path->getP1y());
            if(dis < PRECMD_RANGE)
            {
                actionpoint = path->getEnd();
            }
            else
            {
                if(func_dis(path->getP2x(), path->getP2y(), startstation->getRealX(), startstation->getRealY()) > START_RANGE)
                {
                    actionpoint = path->getEnd();
                }
                break;
            }
        }
        combined_logger->info("action station:{0}", actionpoint);

        actionFlag = false;
        break;
    }
    default:
        actionFlag = true;
        break;
    }
    //for test
    //    startLift = true;
    //告诉小车接下来要执行的路径
    std::vector<int> exelines;
    unsigned int start = 0;

    do
    {
        double speed;
        exelines.clear();
        for(unsigned int i = start; i < lines.size(); i++)
        {
            int line = lines.at(i);
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(-line);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_DyPath)continue;

            DyMapPath *dypath = static_cast<DyMapPath *>(spirit);

            start++;

            if(dypath->getP1f() == dypath->getP2f())
            {
                if(exelines.size() == 0)
                {
                    speed = dypath->getSpeed();
                    exelines.push_back(line);
                }
                else if(speed * dypath->getSpeed() < 0)
                {
                    goStation(exelines, true);
                    exelines.clear();
                    speed = dypath->getSpeed();
                    exelines.push_back(line);
                }
                else
                {
                    exelines.push_back(line);
                }
            }
            else
            {
                goStation(exelines, true);
                /*
                do
                {

                }while(!goElevator(dypath->getStart(), dypath->getEnd(), dypath->getP1f(), dypath->getP2f(),12));
*/
                break;
            }
        }

    }while(start != lines.size());

    if(exelines.size())
    {
        goStation(exelines, true);
    }
}
//上电梯
bool DyForklift::goElevator(int startStation,  int endStation, int from, int to, int eleID)
{
    //TODO 电梯功能完善
    combined_logger->info("goElevator:from {0} to {1}", startStation, endStation);

    Elevator ele(eleID, "ele_0", "127.0.0.1", 8889);
    ele.init();

    while (!ele.IsConnected())
        std::this_thread::sleep_for(std::chrono::microseconds(30));
    int agv  = id;
    // 请求某电梯 (30s超时)
    int elevator;
    do
    {
        elevator= ele.RequestTakeElevator(0, from, eleID, agv, 30);
    }while(elevator == -1);
    if (elevator != -1) {
        // 乘梯应答
        ele.TakeEleAck(from, to, eleID, agv);
        // 等待电梯的进入指令 (30s超时)
        if (ele.ConfirmEleInfo(from, to, eleID, agv, 30)) {
            // todo: 此时agv可以进入, 进入过程每5秒发送一次乘梯应答
            //let agv in
            this->move(0.1, 1.5);

            // 直到完全进入, agv发送进入电梯应答, 电梯开始运行直到到达目标楼层
            if (ele.AgvEnterUntilArrive(from, to, eleID, agv, 30)) {
                // todo: 此时agv可以离开, 离开过程每5秒发送一次离开指令
                //
                //wait agv out
                this->move(-0.1, 1.5);

                // 直到完全离开, 发送离开应答结束乘梯流程
                ele.AgvLeft(from, to, eleID, agv, 30);
                return true;
            }
        }
        //
    }

    return false;
}

//移动至指定站点
void DyForklift::goStation(std::vector<int> lines,  bool stop)
{
    MapPoint *start;
    MapPoint *end;

    //    std::vector<Pose2D> agv_path;
    //    string goal;

    combined_logger->info("dyForklift goStation");
    std::stringstream body;

    int endId;
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int startId = path->getStart();
        endId = path->getEnd();

        MapSpirit *spirit_dy = MapManager::getInstance()->getMapSpiritById(-line);
        DyMapPath *dy_path = static_cast<DyMapPath *>(spirit_dy);

        //获取站点：
        MapSpirit *spirit_start = MapManager::getInstance()->getMapSpiritById(startId);
        if (spirit_start == nullptr || spirit_start->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        MapSpirit *spirit_end = MapManager::getInstance()->getMapSpiritById(endId);
        if (spirit_end == nullptr || spirit_end->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        start = static_cast<MapPoint *>(spirit_start);
        start->getRealX();
        start->getName();
        start->getRealY();

        end = static_cast<MapPoint *>(spirit_end);
        end->getRealX();
        end->getName();
        end->getRealY();

//        combined_logger->info("dyForklift goStation start: " + start->getName());
//        combined_logger->info("dyForklift goStation end: " + end->getName());
        combined_logger->info("dyForklift goStation {0} -> {1}: ({2},{3},{4})->({5},{6},{7})", start->getName(), end->getName(), dy_path->getP1x(),dy_path->getP1y(), dy_path->getP1a(), dy_path->getP2x(), dy_path->getP2y(), dy_path->getP2a());
        //        goal = end->getName();

        float speed = dy_path->getSpeed();
        if(!body.str().length())
        {
            //add current pos
            body<<FORKLIFT_MOVE<<"|"<<speed<<","<<m_currentPos.m_x<<","<<m_currentPos.m_y<<","<<m_currentPos.m_theta*57.3<<","<<m_currentPos.m_floor<<",";
            //            \<<"|"<<speed<<dy_path->getP1x()/100.0<<","<<dy_path->getP1y()/100.0<<","<<dy_path->getP1a()<<","<<dy_path->getP1f()<<","<<dy_path->getPathType();
        }
        body<<dy_path->getPathType()<<"|"<<speed<<","<<dy_path->getP2x()/100.0<<","<<-dy_path->getP2y()/100.0<<","<<dy_path->getP2a()/10.0<<","<<dy_path->getP2f()<<",";
        //for test ---模拟结束后的小车位置
        //        this->lastStation = endId;
        //        this->nowStation = endId;

    }
    body<<"1";

    //    std::stringstream ss;
    //    ss<<"*";
    //    //时间戳
    //    ss.fill('0');
    //    ss.width(6);
    //    time_t   TimeStamp = clock();
    //    ss<<TimeStamp;
    //    //长度
    //    ss.fill('0');
    //    ss.width(4);
    //    ss<<(body.str().length()+10);
    //    ss<<body.str();
    //    ss<<"#";

    resend(body.str().c_str(),body.str().length());

    do
    {

    }while(this->nowStation != endId || !isFinish());
    combined_logger->info("nowStation = {0}, endId = {1}", this->nowStation, endId);

    //    if(agv_path.size() > 0)
    //    {
    //        setAgvPath(agv_path);
    //    }
    //    else
    //    {
    //        combined_logger->error("DyForklift goStation no path...");
    //        return;
    //    }
    //startTask(goal);
}
void DyForklift::setQyhTcp(qyhnetwork::TcpSocketPtr _qyhTcp)
{
    m_qTcp = _qyhTcp;
}
//发送消息给小车
bool DyForklift::send(const char *data, int len)
{
    QByteArray sendBody(data);
    if(sendBody.size() != len)
    {
        combined_logger->error("send length error");
        return false;
    }
    QByteArray sendContent = transToFullMsg(sendBody);
    if(FORKLIFT_HEART != sendContent.mid(11,2).toInt())
    {
        combined_logger->info("send to agv{0}:{1}", id, sendContent.data());
    }

    char * temp = new char[len+13];
    strcpy(temp, sendContent.data());
    bool res = m_qTcp->doSend(temp, len+12);
    if(!res)
    {
        combined_logger->info("send failed");
    }
    DyMsg msg;
    msg.msg = std::string(temp);
    msg.waitTime = 0;
    m_unRecvSend[stoi(msg.msg.substr(1,6))] = msg;
    int msgType = std::stoi(msg.msg.substr(11,2));
    if(FORKLIFT_STARTREPORT != msgType && FORKLIFT_HEART != msgType)
    {
        m_unFinishCmd[msgType]= msg;
    }
    delete temp;
    return res;
}

//开始上报
bool DyForklift::startReport(int interval)
{
    std::stringstream body;
    body<<FORKLIFT_STARTREPORT;
    body.fill('0');
    body.width(5);
    body<<interval;

    //eg:*12345600172100100#
    return resend(body.str().c_str(), body.str().length());
}

//结束上报
bool DyForklift::endReport()
{
    std::stringstream body;
    body<<FORKLIFT_ENDREPORT;
    //eg:*123456001222#
    return resend(body.str().c_str(), body.str().length());
}

//判断小车命令是否执行结束
bool DyForklift::isFinish()
{
    return !m_unFinishCmd.size();
}

bool DyForklift::isFinish(int cmd_type)
{
    std::map<int, DyMsg>::iterator iter = m_unFinishCmd.find(cmd_type);
    if(iter != m_unFinishCmd.end())
    {
        return false;
    }
    else
    {
        return true;
    }
}
