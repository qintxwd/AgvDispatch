#include "atforklift.h"
#include "../common.h"
#include "../mapmap/mappoint.h"
//#include <QByteArray>
//#include <QString>
//#define RESEND
//#define HEART
#define TEST
//TODO 移动后的laststation更新
AtForklift::AtForklift(int id, std::string name, std::string ip, int port) :
    Agv(id, name, ip, port)
{
    startpoint = -1;
    actionpoint = -1;
    status = Agv::AGV_STATUS_NOTREADY;
    pausedFlag = false;
    sendPause = true;//表示发送了暂停指令、false表示发送了暂停指令
    turnResult = true;
    init();
}

void AtForklift::init() {

#ifdef RESEND
    g_threadPool.enqueue([&, this] {
        while (true) {
            std::map<int, DyMsg >::iterator iter;
            if (msgMtx.try_lock())
            {
                for (iter = this->m_unRecvSend.begin(); iter != this->m_unRecvSend.end(); iter++)
                {
                    iter->second.waitTime++;
                    //TODO waitTime>n resend
                    if (iter->second.waitTime > MAX_WAITTIMES)
                    {
                        char* temp = new char[iter->second.msg.length() + 1];
                        strcpy(temp, iter->second.msg.data());
                        bool ret = this->m_qTcp->doSend(temp, iter->second.msg.length());
                        combined_logger->info("resend:{0}, waitTime:{1}, result:{2}", iter->first, iter->second.waitTime, ret ? "success" : "fail");

                        delete[] temp;
                        iter->second.waitTime = 0;
                    }
                }
                msgMtx.unlock();
            }
            usleep(500000);

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

void AtForklift::onTaskStart(AgvTaskPtr _task)
{
    if (_task != nullptr)
    {
        status = Agv::AGV_STATUS_TASKING;
    }
}

void AtForklift::onTaskFinished(AgvTaskPtr _task)
{

}

bool AtForklift::resend(const std::string &msg) {
    if (msg.length()<= 0)return false;
    bool sendResult = send(msg);
    //    bool sendResult = true;

    int resendTimes = 0;
    while (!sendResult && ++resendTimes < maxResendTime) {
        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(500));
        sendResult = send(msg);
    }
    return sendResult;
}

bool AtForklift::fork(int params)
{
    m_lift_height = params;
    std::stringstream body;
    body << ATFORKLIFT_FORK_LIFT;
    body.width(4);
    body.fill('0');
    body << params;
    return resend(body.str());
    //return resend(body.str().c_str(), body.str().length());
}

bool AtForklift::forkAdjust(int params)
{
    std::stringstream body;
    body << ATFORKLIFT_FORK_ADJUST;
    body << params;
    return resend(body.str());
    //return resend(body.str().c_str(), body.str().length());
}

bool AtForklift::forkAdjustAll(int params, int final_height)
{
    std::stringstream body;
    body << ATFORKLIFT_FORK_ADJUSTALL;
    body << params;
    body.width(4);
    body.fill('0');
    body << final_height;
    m_lift_height = final_height;
    return resend(body.str());
}
bool AtForklift::heart()
{
    std::stringstream body;
    body.fill('0');
    body.width(2);
    body << ATFORKLIFT_HEART;
    body << m_lift_height;
    return resend(body.str());
}

Pose4D AtForklift::getPos()
{
    return m_currentPos;
}
int AtForklift::nearestStation(int x, int y, int a, int floor)
{
    int minDis = -1;
    int min_station = -1;
    std::vector<int> stations = MapManager::getInstance()->getStations(floor);
    for (auto station : stations) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;
        MapPoint *point = static_cast<MapPoint *>(spirit);
        long dis = pow(x - point->getRealX(), 2) + pow(y - point->getRealY(), 2);
        if ((min_station == -1 || minDis > dis) && point->getRealA() - a < 30)
        {
            minDis = dis;
            min_station = point->getId();
        }
    }
    return min_station;
}

//解析小车上报消息
void AtForklift::onRead(const char *data, int len)
{
    if (data == NULL || len <= 0)return;
    std::string msg(data, len);
    int length = std::stoi(msg.substr(6, 4));
    if (length != len)
    {
        return;
    }
    int mainMsg = std::stoi(msg.substr(10, 2));
    std::string body = msg.substr(12);

    if (ATFORKLIFT_POS != mainMsg)
    {
        combined_logger->info("agv{0} recv data:{1}", id, data);
    }
    //解析小车发送的消息
    switch (mainMsg) {
    case ATFORKLIFT_POS:
        //小车上报的位置信息
    {
        //*1234560031290|0|0|0.1,0.2,0.3,4#
        std::vector<std::string> all = split(body, "|");
        if (all.size() == 4)
        {
            //status = std::stoi(all[0]);
            //任务线程需要根据此状态判断小车是否在执行任务，不能赋值
            std::vector<std::string> temp = split(all[3], ",");
            if (temp.size() == 4)
            {
                m_currentPos = Pose4D(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]), std::stoi(temp[3]));

                x = m_currentPos.m_x * 100;
                y = -m_currentPos.m_y * 100;
                theta = m_currentPos.m_theta*57.3;
                if (AGV_STATUS_NOTREADY == status)
                {
                    //find nearest station
                    nowStation = nearestStation(x, y, theta, m_currentPos.m_floor);
                    if (nowStation != -1){
                        status = AGV_STATUS_IDLE;
                        onArriveStation(nowStation);
                        //occur now station
                        MapManager::getInstance()->addOccuStation(nowStation,shared_from_this());
                    }
                }
                else
                {
                    arrve(x, y);
                }
            }
        }

        break;
    }
    case ATFORKLIFT_BATTERY:
    {
        //小车上报的电量信息
        m_power = std::stoi(body);
        break;
    }
    case ATFORKLIFT_FINISH:
    {
        //小车上报运动结束状态或自定义任务状态
        if (1 <= std::stoi(body.substr(2)))
        {
            //command finish
            std::map<int, DyMsg>::iterator iter = m_unFinishCmd.find(std::stoi(body.substr(0, 2)));
            if (iter != m_unFinishCmd.end())
            {
                m_unFinishCmd.erase(iter);
            }
        }
        break;
    }
    case ATFORKLIFT_MOVE:
    case ATFORKLIFT_TURN:
    case ATFORKLIFT_FORK_LIFT:
    case ATFORKLIFT_STARTREPORT:
    {
        msgMtx.lock();
        //command response
        std::map<int, DyMsg>::iterator iter = m_unRecvSend.find(std::stoi(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        msgMtx.unlock();
        break;
    }

    case ATFORKLIFT_MOVE_NOLASER:
    {
        msgMtx.lock();
        //command response
        std::map<int, DyMsg>::iterator iter = m_unRecvSend.find(std::stoi(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        msgMtx.unlock();

        if (std::stoi(msg.substr(0, 7)) == 0){
            pausedFlag = false;
        }else  if (std::stoi(msg.substr(0, 7)) == 1){
            pausedFlag = true;
        }
        break;
    }
    default:
        break;
    }
}
//判断小车是否到达某一站点
void AtForklift::arrve(int x, int y) {

    auto mapmanagerptr = MapManager::getInstance();

    //1.does leave station
    if(nowStation>0){
        //how far from current pos to nowStation
        MapSpirit *spirit = mapmanagerptr->getMapSpiritById(nowStation);
        if (spirit != nullptr && spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point)
        {
            auto point = static_cast<MapPoint *>( spirit );
            if (func_dis(x, y, point->getRealX(), point->getRealY()) > AT_START_RANGE) {
                //too far leave station
                onLeaveStation(nowStation);
            }
        }
    }

    //2.did agv arrive a station
    int arriveId = -1;
    double minDis = DISTANCE_INFINITY_DOUBLE;
    stationMtx.lock();
    for(auto station:excutestations){
        MapPoint *point = mapmanagerptr->getPointById(station);
        if (point == nullptr)continue;

        auto dis = func_dis(x, y, point->getRealX(), point->getRealY());
        if(dis<minDis && dis<AT_PRECISION && station != nowStation){
            minDis = dis;
            arriveId = station;
        }
    }
    stationMtx.unlock();
    if(arriveId!=-1){
        onArriveStation(arriveId);
    }

    //4. did agv arrive actionpoint
    if ((TASK_PICK == task_type || TASK_PUT == task_type) && !actionFlag)
    {
        //std::vector<int> stations = MapManager::getInstance()->getStations(m_currentPos.m_floor);
        //int samefloor = std::count(stations.begin(), stations.end(), actionpoint);
        //if(samefloor)
        bool sameFloor = MapManager::getInstance()->isSameFloor(m_currentPos.m_floor, actionpoint);
        if (sameFloor)
        {
            MapPoint *point = MapManager::getInstance()->getPointById(actionpoint);
            if (point == nullptr)return;

            MapPoint *start = MapManager::getInstance()->getPointById(startpoint);
            if (point == nullptr)return;

            if (func_dis(x, y, point->getRealX(), point->getRealY()) < AT_PRECISION && func_dis(x, y, start->getRealX(), start->getRealY()) > 200) {
                actionFlag = true;
                startLift = true;
                AgvTaskPtr currentTask = this->getTask();
                AgvTaskNodePtr currentTaskNode = currentTask->getTaskNodes().at(currentTask->getDoingIndex());
                fork(stoi(split(currentTaskNode->getParams(), ";").at(0)));
                return;
            }
        }
    }

    //3.did agv lift to move heigth
    if (!startLift)
    {
        MapPoint *point = mapmanagerptr->getPointById(startpoint);

        if(point!=nullptr){
            if (func_dis(x, y, point->getRealX(), point->getRealY()) > AT_START_RANGE) {
                startLift = true;
                fork(AT_MOVE_HEIGHT);
            }
        }
    }
}
//进电梯时用
bool AtForklift::move(float speed, float distance)
{
    std::stringstream body;
    body << ATFORKLIFT_MOVE_NOLASER << speed << "|" << distance;
    return resend(body.str());
}

//执行路径规划结果
void AtForklift::excutePath(std::vector<int> lines)
{
    combined_logger->info("agv id:{0} excutePath now ", id);

    std::stringstream ss;
    stationMtx.lock();
    excutestations.clear();
    m_unFinishCmd.clear();
    m_unRecvSend.clear();
    excutestations.push_back(nowStation);
    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int endId = path->getEnd();
        excutestations.push_back(endId);
        excutespaths.push_back(line);
        ss<<path->getName()<<"  ";
    }
    stationMtx.unlock();

    combined_logger->info("excutePath: {0}", ss.str());

    actionpoint = 0;
    MapPoint *startstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.front()));
    MapPoint *endstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.back()));
    startpoint = startstation->getId();
    startLift = false;

    AgvTaskPtr currentTask = this->getTask();
    AgvTaskNodePtr currentTaskNode = currentTask->getTaskNodes().at(currentTask->getDoingIndex());
    task_type = currentTaskNode->getType();
    combined_logger->info("taskType: {0}", task_type);

    m_fix_x = 0;
    m_fix_y = 0;
    m_fix_a = 0;

    if (TASK_MOVE != task_type)
    {
        auto params = split(currentTaskNode->getParams(), ";");
        m_lift_height = stoi(params.at(0));
        if (params.size() >= 5) {
            m_fix_x = stoi(params.at(2));
            m_fix_y = stoi(params.at(3));
            m_fix_a = stoi(params.at(4));
        }
    }
    else
    {
        m_lift_height = 100;
    }

    switch (task_type) {
    case TASK_PICK:
    case TASK_PUT:
    {
        //TODO
        //多层楼需要修改

        if (func_dis(startstation->getX(), startstation->getY(), endstation->getX(), endstation->getY()) < AT_START_RANGE * 2)
        {
            startLift = true;
        }
        actionpoint = startstation->getId();

        double dis = 0;
        for (int i = lines.size() - 1; i >= 0; i--) {
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(lines.at(i));
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

            auto path = static_cast<MapPath *>(spirit);
            auto path_start = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(path->getStart()));
            auto path_end = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(path->getEnd()));
            if (path_start == nullptr || path_start->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;
            if (path_end == nullptr || path_end->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;

            dis += func_dis(path_end->getRealX(), path_end->getRealY(), path_start->getRealX(), path_start->getRealY());
            if (dis < AT_PRECMD_RANGE)
            {
                actionpoint = path->getEnd();
            }
            else
            {
                if (func_dis(path_end->getRealX(), path_end->getRealY(), startstation->getRealX(), startstation->getRealY()) > 200)
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
    //      startLift = true;
    //告诉小车接下来要执行的路径
    std::vector<int> exelines;
    unsigned int start = 0;

    do
    {
        double speed = 0.4;
        exelines.clear();
        for (unsigned int i = start; i < lines.size(); i++)
        {
            int line = lines.at(i);
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;

            auto path = static_cast<MapPath *>(spirit);

            start++;

            if (path->getPathType() != MapPath::Map_Path_Type_Between_Floor)
            {
                if (exelines.size() == 0)
                {
                    speed = path->getSpeed();
                    exelines.push_back(line);
                }
                else if (speed * path->getSpeed() < 0)
                {
                    goStation(exelines, false);
                    exelines.clear();
                    speed = path->getSpeed();
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

    } while (start != lines.size());

    if (exelines.size())
    {
        goStation(exelines, true);
    }
    combined_logger->info("excute path finish");
}

//移动至指定站点
void AtForklift::goStation(std::vector<int> lines, bool stop)
{
    MapPoint *start;
    MapPoint *end;

    MapManagerPtr mapmanagerptr = MapManager::getInstance();

    combined_logger->info("atForklift goStation");
    std::stringstream body;

    int endId = 0;
    for (auto i = 0; i < lines.size(); ++i) {
        auto line = lines[i];
        MapSpirit *spirit = mapmanagerptr->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        int startId = path->getStart();
        endId = path->getEnd();

        //获取站点：
        MapSpirit *spirit_start = mapmanagerptr->getMapSpiritById(startId);
        if (spirit_start == nullptr || spirit_start->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        MapSpirit *spirit_end = mapmanagerptr->getMapSpiritById(endId);
        if (spirit_end == nullptr || spirit_end->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        start = static_cast<MapPoint *>(spirit_start);
        end = static_cast<MapPoint *>(spirit_end);

        //combined_logger->info("atForklift goStation {0} -> {1}: ({2},{3},{4})->({5},{6},{7})", start->getName(), end->getName(), start->getRealX(), start->getRealY(), start->getRealA(), end->getRealX(), end->getRealY(), end->getRealA());

        float speed = path->getSpeed();
        if (!body.str().length())
        {
            //add current pos
            body << ATFORKLIFT_MOVE << "|" << speed << "," << m_currentPos.m_x << "," << m_currentPos.m_y << "," << m_currentPos.m_theta*57.3 << "," << m_currentPos.m_floor << ",";
            //            \<<"|"<<speed<<dy_path->getP1x()/100.0<<","<<dy_path->getP1y()/100.0<<","<<dy_path->getP1a()<<","<<dy_path->getP1f()<<","<<dy_path->getPathType();
        }


        //combined_logger->info("i={0},line={1},m_fix_x={2},m_fix_y={3},m_fix_a={4},end.realX={5},end.realX={6},end.realA={7},end of lines = {8}", i, line, m_fix_x, m_fix_y, m_fix_a, end->getRealX(), end->getRealY(), end->getRealA(), excutespaths.back());

        int pathtype  = 3;
        if(path->getPathType() == MapPath::Map_Path_Type_Line){
            pathtype = 1;
        }
        //        else if(path->getPathType() == MapPath::Map_Path_Type_Between_Floor){
        //            pathtype = 3;
        //        }

        if (line == excutespaths.back()
                && !(m_fix_a == 0 && m_fix_x == 0 && m_fix_y == 0)
                && func_dis(end->getRealX(), end->getRealY(), m_fix_x, m_fix_y) < 200)
        {
            body << pathtype << "|" << speed << "," << m_fix_x / 100.0 << "," << -m_fix_y / 100.0 << "," << m_fix_a / 10.0 << "," << mapmanagerptr->getFloor(endId) << ",";
        }
        else {
            body << pathtype << "|" << speed << "," << end->getRealX() / 100.0 << "," << -end->getRealY() / 100.0 << "," << end->getRealA() / 1.0 << "," << mapmanagerptr->getFloor(endId) << ",";
        }
    }
    body << "1";

    //TODO 掉线处理
    resend(body.str());

    do
    {
        //wait for move finish
        usleep(50000);

        //如果中途因为block被暂停了，那么就判断block是否可以进入了，如果可以进入，那么久要发送resume
        if (pausedFlag && sendPause)
        {
            usleep(500000);
            //判断block是否可以进入
            std::vector<int> bs;
            if(nowStation!=0){
                int pId = -1;
                stationMtx.lock();
                for (int i = 0; i < excutespaths.size(); ++i) {
                    auto path = mapmanagerptr->getPathById(excutespaths[i]);
                    if (path!=nullptr && path->getStart() == nowStation) {
                        pId = path->getId();
                        break;
                    }
                }
                stationMtx.unlock();
                bs = mapmanagerptr->getBlocks(pId);
            }
            else {
                bs = mapmanagerptr->getBlocks(nextStation);
            }
            bool canResume = true;
            for(auto b:bs){
                if (!mapmanagerptr->blockPassable(b,getId())) {
                    canResume = false;
                    break;
                }
            }
            if(canResume)
                resume();

        }
    } while (this->nowStation != endId || !isFinish(ATFORKLIFT_MOVE));
    combined_logger->info("nowStation = {0}, endId = {1}", this->nowStation, endId);

    if (stop)
    {
        while (!isFinish())
        {
            //wait for all finish
            usleep(50000);
        }
    }
}
void AtForklift::setQyhTcp(SessionPtr _qyhTcp)
{
    m_qTcp = _qyhTcp;
}
//发送消息给小车
bool AtForklift::send(const std::string &data)
{
    std::string sendContent = transToFullMsg(data);

    if (ATFORKLIFT_HEART != stringToInt(sendContent.substr(11, 2)))
    {
        combined_logger->info("send to agv{0}:{1}", id, sendContent);
    }
    bool res = m_qTcp->doSend(sendContent.c_str(), sendContent.length());
    if(!res){
        combined_logger->info("send to agv msg fail!");
    }
    DyMsg msg;
    msg.msg = sendContent;
    msg.waitTime = 0;
    msgMtx.lock();
    m_unRecvSend[stoi(msg.msg.substr(1, 6))] = msg;
    msgMtx.unlock();
    int msgType = std::stoi(msg.msg.substr(11, 2));
    if (ATFORKLIFT_STARTREPORT != msgType && ATFORKLIFT_HEART != msgType)
    {
        m_unFinishCmd[msgType] = msg;
    }
    return res;
}

//开始上报
bool AtForklift::startReport(int interval)
{
    std::stringstream body;
    body << ATFORKLIFT_STARTREPORT;
    body.fill('0');
    body.width(5);
    body << interval;

    //eg:*12345600172100100#
    return resend(body.str());
}

//结束上报
bool AtForklift::endReport()
{
    std::stringstream body;
    body << ATFORKLIFT_ENDREPORT;
    //eg:*123456001222#
    return resend(body.str());
}

//判断小车命令是否执行结束
bool AtForklift::isFinish()
{
    return !m_unFinishCmd.size();
}

bool AtForklift::isFinish(int cmd_type)
{
    std::map<int, DyMsg>::iterator iter = m_unFinishCmd.find(cmd_type);
    if (iter != m_unFinishCmd.end())
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool AtForklift::pause()
{
    combined_logger->debug("==============agv:{0} paused!",getId());
    std::stringstream body;
    body << ATFORKLIFT_MOVE_NOLASER;
    body << 1;
    sendPause = true;
    return resend(body.str());
}

bool AtForklift::resume()
{
    combined_logger->debug("==============agv:{0} resume!",getId());
    std::stringstream body;
    body << ATFORKLIFT_MOVE_NOLASER;
    body << 0;
    sendPause = false;
    return resend(body.str());
}
