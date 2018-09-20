#include "dyforklift.h"
#include "../common.h"
#include "../mapmap/mappoint.h"
#include "../device/elevator/elevator.h"
#include "charge/chargemachine.h"
#include "../device/elevator/elevatorManager.h"
#include "../mapmap/blockmanager.h"
#define RESEND
//#define HEART

DyForklift::DyForklift(int id, std::string name, std::string ip, int port):
    Agv(id,name,ip,port),
    m_qTcp(nullptr),
    firstConnect(true)
{
    //    startpoint = -1;
    //    actionpoint = -1;
    pauseFlag = false;
    sendPause = false;
    status = Agv::AGV_STATUS_UNCONNECT;
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
    std::thread([&]{
        while (!g_quit) {
            if(m_qTcp == nullptr){
                //已经掉线了
                sleep(1);
                continue;
            }
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
                        //TODO 判断连接是否有效
                        if(m_qTcp)
                        {
                            int ret = m_qTcp->doSend(iter->second.msg.c_str(), iter->second.msg.length());
                            combined_logger->info("resend:{0}, waitTime:{1}, result:{2}", iter->first, iter->second.waitTime, ret ? "success" : "fail");
                            iter->second.waitTime = 0;
                        }
                    }
                }

                msgMtx.unlock();
            }
            sleep(1);
        }
    }).detach();
#endif
    //#ifdef HEART
    //    g_threadPool.enqueue([&, this] {
    //        while (true) {
    //            heart();
    //            usleep(500000);

    //        }
    //    });
    //#endif
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

bool DyForklift::resend(const std::string &msg){
    if (msg.length()<= 0)return false;
    bool sendResult = send(msg);
    int resendTimes = 0;

    if(currentTask==nullptr){
        while(!sendResult && ++resendTimes< maxResendTime){
            std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(500));
            sendResult = send(msg);
        }
    }else{
        // doing task!!!!!!!!!!!
        // no return until send ok except task is cancel or task is error
        while(!sendResult && !currentTask->getIsCancel() && currentTask->getErrorCode() == 0){
            std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(500));
            sendResult = send(msg);
        }
    }
    return sendResult;
}

bool DyForklift::fork(int params)
{
    m_lift = (params!= 0) ? true: false;
    std::stringstream body;
    body<<FORKLIFT_FORK;
    body.width(2);
    body.fill('0');
    body<<params;
    return resend(body.str());
}

//bool DyForklift::heart()
//{
//    std::stringstream body;
//    body.fill('0');
//    body.width(2);
//    body<<FORKLIFT_HEART;
//    body<<m_lift<<m_lift;
//    return resend(body.str().c_str());
//}

Pose4D DyForklift::getPos()
{
    return m_currentPos;
}

int DyForklift::nearestStation(int x, int y, int a, int floor)
{
    int minDis = -1;
    int min_station = -1;
    auto mapmanager = MapManager::getInstance();
    std::vector<int> stations = mapmanager->getStations(floor);
    for (auto station : stations) {
        MapPoint *point = mapmanager->getPointById(station);
        if (point == nullptr)continue;
        long dis = pow(x-point->getRealX(),2)+pow(y-point->getRealY(),2);
        if ((min_station == -1 || minDis > dis) && point->getRealA() - a < 40)
        {
            minDis = dis;
            min_station = point->getId();
        }
    }
    return min_station;
}

//解析小车上报消息
void DyForklift::onRead(const char *data,int len)
{
    if(data == NULL || len < 10)return ;

    if(firstConnect)
    {
        firstConnect = false;
        std::stringstream body;
        body << FORKLIFT_MOVE_NOLASER;
        body << FORKLIFT_NOLASER_CLEAR_TASK;
        send(body.str());
    }

    std::string msg(data,len);
    int length = stringToInt(msg.substr(6, 4));
    if(length != len && length < 12)
    {
        return;
    }

    int mainMsg = stringToInt(msg.substr(10, 2));
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
            //status = stringToInt(all[0]);
            //任务线程需要根据此状态判断小车是否在执行任务，不能赋值
            std::vector<std::string> temp = split(all[3], ",");
            if(temp.size() == 4)
            {
                m_currentPos = Pose4D(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]), stringToInt(temp[3]));

                x = m_currentPos.m_x*100;
                y = -m_currentPos.m_y*100;
                theta = -m_currentPos.m_theta*57.3;
                floor = m_currentPos.m_floor;
                if(AGV_STATUS_NOTREADY == status)
                {
                    //find nearest station
                    nowStation = nearestStation(m_currentPos.m_x*100, -m_currentPos.m_y*100, m_currentPos.m_theta, m_currentPos.m_floor);
                    if(nowStation != -1){
                        status = AGV_STATUS_IDLE;
                        onArriveStation(nowStation);
                    }
                }
                else
                {
                    arrve(m_currentPos.m_x*100, -m_currentPos.m_y*100);
                }

            }
        }

        break;
    }
    case FORKLIFT_WARN:
    {
        std::vector<std::string> warn = split(body, "|");
        WarnSt tempWarn;
        if(warn.size() != 14)
        {
            break;
        }
        tempWarn.iLocateSt = stringToInt(warn.at(0));            //定位丢失,1表示有告警，0表示无告警
        tempWarn.iSchedualPlanSt = stringToInt(warn.at(1));             //调度系统规划错误,1表示有告警，0表示无告警
        tempWarn.iHandCtrSt = stringToInt(warn.at(2)); 	            //模式切换（集中调度，手动），0表示只接受集中调度，1表示只接受手动控制
        tempWarn.iObstacleLaserSt = stringToInt(warn.at(3));           //激光避障状态 0： 远 1： 中 2：近
        tempWarn.iScramSt = stringToInt(warn.at(4));           //紧急停止状态 0:非 1：紧急停止
        tempWarn.iTouchEdgeSt = stringToInt(warn.at(5));       //触边状态    0：正常 1：触边
        tempWarn.iTentacleSt = stringToInt(warn.at(6));        //触须状态    0：正常  1：触须
        tempWarn.iTemperatureSt = stringToInt(warn.at(7));     //温度保护状态 0：正常  1：温度保护
        tempWarn.iDriveSt = stringToInt(warn.at(8));           //驱动状态    0：正常  1：异常
        tempWarn.iBaffleSt = stringToInt(warn.at(9));          //挡板状态    0：正常  1：异常
        tempWarn.iBatterySt = stringToInt(warn.at(10));         //电池状态    0：正常  1：过低
        tempWarn.iLocationLaserConnectSt = stringToInt(warn.at(11));         //定位激光连接状态    0： 正常 1:异常
        tempWarn.iObstacleLaserConnectSt  = stringToInt(warn.at(12));         //避障激光连接状态    0： 正常 1：异常
        tempWarn.iSerialPortConnectSt = stringToInt(warn.at(13));            //串口连接状态       0： 正常 1：异常
        if(tempWarn == m_warn)
        {
        }
        else
        {
            m_warn = tempWarn;
            combined_logger->info("agv{1} warn changed:{0}", m_warn.toString(), id);
        }
        break;
    }
    case FORKLIFT_BATTERY:
    {
		//小车上报的电量信息
		if(body.length()>0)    
    	    m_power = stringToInt(body);
        break;
    }
    case FORKLIFT_FINISH:
    {
        //小车上报运动结束状态或自定义任务状态
        if(body.length()<2)return ;
        if(1 == stringToInt(body.substr(2)))
        {
            //command finish
            std::map<int, DyMsg>::iterator iter = m_unFinishCmd.find(stringToInt(body.substr(0,2)));
            if(iter != m_unFinishCmd.end())
            {
                m_unFinishCmd.erase(iter);
            }
        }
        break;
    }
    case FORKLIFT_MOVE:
    case FORKLIFT_FORK:
    case FORKLIFT_STARTREPORT:
    case FORKLIFT_MOVEELE:
    case FORKLIFT_CHARGE:
    case FORKLIFT_INITPOS:
    {
        msgMtx.lock();
        //command response
        std::map<int, DyMsg>::iterator iter = m_unRecvSend.find(stringToInt(msg.substr(0,6)));
        if(iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        msgMtx.unlock();
        break;
    }
    case FORKLIFT_MOVE_NOLASER:
    {
        msgMtx.lock();
        //command response
		//TODO
        std::map<int, DyMsg>::iterator iter = m_unRecvSend.find(stringToInt(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        msgMtx.unlock();
		//TODO:
        if (stringToInt(msg.substr(0, 7)) == 1){
            pauseFlag = sendPause;
        }
        else  if (stringToInt(msg.substr(0, 7)) == 1){
            pauseFlag = false;
            sendPause = false;
        }
        break;
    }
    default:
        break;
    }
}
//判断小车是否到达某一站点
void DyForklift::arrve(int x, int y) {

    auto mapmanagerptr = MapManager::getInstance();

    //1.does leave station
    if(nowStation>0){
        //how far from current pos to nowStation
        auto point = mapmanagerptr->getPointById(nowStation);
        if (point != nullptr)
        {
            if (func_dis(x, y, point->getRealX(), point->getRealY()) > 2*PRECISION) {
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
        if(dis<minDis && dis<PRECISION && station != nowStation && func_angle(-point->getRealA()/10, (int)theta) < ANGLE_PRECISION ){
            minDis = dis;
            arriveId = station;
        }

        if(station == currentEndStation)
        {
            //已抵达当前路径终点，退出判断
            break;
        }
    }
    stationMtx.unlock();
    if(arriveId!=-1){
        onArriveStation(arriveId);
    }
}

//执行路径规划结果
void DyForklift::excutePath(std::vector<int> lines)
{
    combined_logger->info("dyForklift{0} excutePath", id);

    std::stringstream ss;
    stationMtx.lock();
    excutestations.clear();
    excutespaths.clear();
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
    //    actionpoint = 0;
    //MapPoint *startstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.front()));
    //MapPoint *endstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.back()));
    //    startpoint = startstation->getId();
    //TODO
    //    startLift = true;

    AgvTaskPtr currentTask =this->getTask();

    task_type = currentTask->getTaskNodes().at(currentTask->getDoingIndex())->getType();
    //    combined_logger->info("taskType: {0}", task_type);

    //    switch (task_type) {
    //    case TASK_PICK:
    //    {
    //        //TODO
    //        //多层楼需要修改

    //        if(func_dis(startstation->getX(), startstation->getY(), endstation->getX(), endstation->getY()) < START_RANGE*2)
    //        {

    //            startLift = true;
    //        }
    //        actionpoint = startstation->getId();

    //        double dis = 0;
    //        for (int i = lines.size()-1; i >= 0; i--) {
    //            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(-lines.at(i));
    //            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_DyPath)continue;

    //            DyMapPath *path = static_cast<DyMapPath *>(spirit);
    //            dis += func_dis(path->getP2x(), path->getP2y(), path->getP1x(), path->getP1y());
    //            if(dis < PRECMD_RANGE)
    //            {
    //                actionpoint = path->getEnd();
    //            }
    //            else
    //            {
    //                if(func_dis(path->getP2x(), path->getP2y(), startstation->getRealX(), startstation->getRealY()) > START_RANGE)
    //                {
    //                    actionpoint = path->getEnd();
    //                }
    //                break;
    //            }
    //        }
    //        combined_logger->info("action station:{0}", actionpoint);

    //        actionFlag = false;
    //        break;
    //    }
    //    default:
    //        actionFlag = true;
    //        break;
    //    }
    //for test
    //    startLift = true;
    //告诉小车接下来要执行的路径
    //    if(0)
    bool needElevator = isNeedTakeElevator(lines);
    if(!needElevator)//不需要乘电梯
    {
        combined_logger->info("DyForklift, excutePath, don't need to take elevator");
        //告诉小车接下来要执行的路径
        goSameFloorPath(lines);
    }//需要乘电梯
    else
    {
        goElevator(lines);
    }
}

void DyForklift::goSameFloorPath(std::vector<int> lines)
{
    std::vector<int> exelines;
    unsigned int start = 0;
    auto mapmanager = MapManager::getInstance();
    do
    {
        double speed = 0.4;
        exelines.clear();
        for(unsigned int i = start; i < lines.size(); i++)
        {
            int line = lines.at(i);

            auto path = mapmanager->getPathById(line);

            start++;

            if(exelines.size() == 0)
            {
                speed = path->getSpeed();
                exelines.push_back(line);
            }
            else if(speed * path->getSpeed() < 0)
            {
                goStation(exelines, true, FORKLIFT_MOVE);
                exelines.clear();
                speed = path->getSpeed();
                exelines.push_back(line);
            }
            else
            {
                exelines.push_back(line);
            }
        }

    }while(start != lines.size());

    if(exelines.size())
    {
        goStation(exelines, true,FORKLIFT_MOVE);
    }
}
//上电梯
void DyForklift::goElevator(std::vector<int> lines)
{
    std::vector<int> lines_to_elevator;//到达电梯口路径
    std::vector<int> lines_enter_elevator;//进入电梯路径
    std::vector<int> lines_take_elevator;//乘电梯到达楼层
    std::vector<int> lines_out_elevator;//出电梯路径
    std::vector<int> lines_leave_elevator;//离开电梯口去目标路径

    lines_to_elevator = getPathToElevator(lines);//到达电梯口路径
    lines_enter_elevator = getPathEnterElevator(lines);//进入电梯路径
    lines_take_elevator = getPathInElevator(lines);//乘电梯到达楼层
    lines_out_elevator  = getPathOutElevator(lines);//出电梯路径
    lines_leave_elevator = getPathLeaveElevator(lines);//离开电梯去目标路径

    int agv_id = this->getId();

    int from_floor = MapManager::getInstance()->getFloor(nowStation);

    int elevator_line = lines_take_elevator.at(0);

    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(elevator_line);
    MapPath *elevator_path = static_cast<MapPath *>(spirit);
    MapSpirit *elevator_spirit = MapManager::getInstance()->getMapSpiritById(elevator_path->getStart());
    MapPoint * elevator_point = static_cast<MapPoint*>(elevator_spirit);
    combined_logger->info("DyForklift,  excutePath, elevatorID: {0}", elevator_point->getLineId());


    int elevator_back = lines_take_elevator.back();
    MapSpirit *spirit_back = MapManager::getInstance()->getMapSpiritById(elevator_back);
    MapPath *elevator_path_back = static_cast<MapPath *>(spirit_back);
    int endId = elevator_path_back->getEnd();

    //根据电梯等待区设置呼叫某个ID电梯
    ElevatorPtr elevator = ElevatorManager::getInstance()->getElevatorById(stoi(elevator_point->getLineId()));

    if(elevator == nullptr)
    {
        combined_logger->error("DyForklift,  excutePath, no elevator!!!!!");
        return;
    }

    int to_floor = MapManager::getInstance()->getFloor(endId);

    combined_logger->info("DyForklift,  excutePath, endId: {0}", endId);

    combined_logger->info("DyForklift,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);


    if(lines_to_elevator.size() > 0)
    {
        goSameFloorPath(lines_to_elevator);//到达电梯口
    }

    //呼叫电梯到达楼层
    int result = -1;
    do
    {
        combined_logger->info("DyForklift,  excutePath, 呼叫电梯到达 {0} 楼, 30s超时继续呼叫电梯 ", from_floor);
        result = elevator->RequestTakeElevator(from_floor, 0, elevator->getId(), agv_id, 30);
    }
    while(result < 0);


    int try_times = 10;
    bool enter_ele = false;
    do
    {
        combined_logger->info("DyForklift,  excutePath, 发送乘梯应答 ");

        elevator->TakeEleAck(from_floor, to_floor, elevator->getId(), agv_id);
        std::cout << "等待电梯的进入指令...." << std::endl;
        combined_logger->info("DyForklift,  excutePath, 发送乘梯应答, 5s超时 ");

        enter_ele = elevator->ConfirmEleInfo(from_floor, to_floor, elevator->getId(), agv_id, 5);
        try_times--;
    }
    while(!enter_ele &&  try_times > 0);

    if( enter_ele == false )
    {
        combined_logger->error("DyForklift,  excutePath, ====no 等待电梯的进入指令....==================");
        return;
    }

    elevator->StartSendThread(lynx::elevator::TakeEleACK,from_floor, to_floor, elevator->getId(), agv_id);

    sleep(3); //等待电梯门完全打开

    if(lines_enter_elevator.size() > 0)
    {
        goStation(lines_enter_elevator, true, FORKLIFT_MOVEELE);//进入电梯
    }

    elevator->StopSendThread();

    bool leave_elevator = false;

    do
    {
        elevator->AgvEnterConfirm(from_floor, to_floor, elevator->getId(), agv_id, 3);

        leave_elevator = elevator->AgvWaitArrive(from_floor, to_floor, elevator->getId(), agv_id, 30);
        if(leave_elevator)
        {
            combined_logger->info("DyForklift,  excutePath, ===电梯到达===");
        }
        else
        {
            combined_logger->error("DyForklift,  excutePath, ===电梯未到达===");
        }
    }
    while(!leave_elevator);

    if(leave_elevator)
    {
        combined_logger->debug("DyForklift,  excutePath, *********************************");
        combined_logger->debug("DyForklift,  excutePath, ******start to change map********");
        combined_logger->debug("DyForklift,  excutePath, *********************************");

        //TODO 切换地图
        setInitPos(endId);

        //通知电梯AGV正在出电梯,离开过程中每5s发送一次【离开指令】，要求内呼等待机器人离开
        elevator->StartSendThread(lynx::elevator::LeftEleCMD,from_floor, to_floor, elevator->getId(), agv_id);

        sleep(5);//等待电梯门完全打开

        //AGV出电梯
        goStation(lines_out_elevator, true, FORKLIFT_MOVEELE);
        //this->startTask("out_elevator");
        elevator->StopSendThread();//stop send 离开指令

        sleep(2);


        if(lines_out_elevator.size() > 0)
        {
            int path_id = lines_out_elevator.at(lines_out_elevator.size()-1);
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(path_id);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            {
                combined_logger->error("DyForklift,  excutePath, !MapSpirit::Map_Sprite_Type_Path");
                return;
            }

            MapPath *path = static_cast<MapPath *>(spirit);

            this->nowStation = path->getEnd();
            combined_logger->debug("DyForklift,  excutePath, AGV出电梯, nowStation: {0}", nowStation);

            this->lastStation = 0;
            this->nextStation = 0;
        }
    }

    //通知电梯AGV完成出电梯
    bool leftEleStatus = elevator->AgvLeft(from_floor, to_floor, elevator->getId(), agv_id, 10);

    /*if(!leftEleStatus)//复位电梯状态, maybe elevator error
        {
            elevator->ResetElevatorState(elevator->getId());
        }*/

    if(lines_leave_elevator.size() > 0)
    {
        combined_logger->debug("DyForklift,  excutePath, 离开电梯口去目标");
        goSameFloorPath(lines_leave_elevator);//离开电梯口去目标
    }
    combined_logger->info("excute path finish");
}

//移动至指定站点
void DyForklift::goStation(std::vector<int> lines,  bool stop, FORKLIFT_COMM cmd)
{
    MapPoint *start;
    MapPoint *end;

    MapManagerPtr mapmanagerptr = MapManager::getInstance();
    BlockManagerPtr blockmanagerptr = BlockManager::getInstance();

    combined_logger->info("dyForklift goStation");
    std::stringstream body;

    int endId = 0;
    for (auto line : lines) {
        MapPath *path = mapmanagerptr->getPathById(line);
        if(path == nullptr)continue;
        int startId = path->getStart();
        endId = path->getEnd();
        start = mapmanagerptr->getPointById(startId);
        if(start == nullptr)continue;
        end = mapmanagerptr->getPointById(endId);
        if(end == nullptr)continue;

        float speed = path->getSpeed();
        if(!body.str().length())
        {
            if(cmd == FORKLIFT_MOVEELE)
            {
                body<<cmd<<"|"<<speed<<","<<end->getRealX() / 100.0<<","<< -end->getRealY() / 100.0<<","<<end->getRealA() / 10.0<<","<<mapmanagerptr->getFloor(endId)<<",";
            }
            else
            {
                //add current pos
                body<<cmd<<"|"<<speed<<","<<m_currentPos.m_x<<","<<m_currentPos.m_y<<","<<m_currentPos.m_theta*57.3<<","<<m_currentPos.m_floor<<",";
                //            \<<"|"<<speed<<dy_path->getP1x()/100.0<<","<<dy_path->getP1y()/100.0<<","<<dy_path->getP1a()<<","<<dy_path->getP1f()<<","<<dy_path->getPathType();
            }
        }
        int type = 1;
        if(path->getPathType() == MapPath::Map_Path_Type_Quadratic_Bezier
                ||path->getPathType() == MapPath::Map_Path_Type_Cubic_Bezier)type = 3;
        if(path->getPathType() == MapPath::Map_Path_Type_Between_Floor)type = 1;
        body<<type<<"|"<<speed<<","<<end->getRealX() / 100.0<<","<< -end->getRealY() / 100.0<<","<<end->getRealA() / 10.0<<","<<mapmanagerptr->getFloor(endId)<<",";
    }
    body<<"1";

    currentEndStation = endId;

    //TODO:
    while(!g_quit && currentTask!=nullptr && !currentTask->getIsCancel()){
        //can start the path?
        bool canGo = true;
        for(auto line:lines){
            auto linePtr = mapmanagerptr->getPathById(line);
            if(linePtr==nullptr)continue;
            auto bs = mapmanagerptr->getBlocks(line);
            if(!blockmanagerptr->tryAddBlockOccu(bs,getId(),line)){
                canGo = false;
                break;
            }

            auto bs2 = mapmanagerptr->getBlocks(linePtr->getEnd());
            if(!blockmanagerptr->tryAddBlockOccu(bs2,getId(),linePtr->getEnd())){
                canGo = false;
            }
            break;
        }
        if(canGo)break;
        usleep(500000);
    }

    if(g_quit || currentTask == nullptr || currentTask->getIsCancel())return ;


    resend(body.str());

    do
    {
        //wait for move finish
        usleep(50000);

        //如果中途因为block被暂停了，那么就判断block是否可以进入了，如果可以进入，那么久要发送resume
        if (sendPause || pauseFlag)
        {
            sleep(1);

            bool canResume = true;

            //could occu current station or path block?
            if(nowStation>0){
                auto bs = mapmanagerptr->getBlocks(nowStation);
                if(!blockmanagerptr->tryAddBlockOccu(bs,getId(),nowStation)){
                    canResume = false;
                }
            }else{
                auto path = mapmanagerptr->getPathByStartEnd(lastStation,nextStation);
                if(path!=nullptr){
                    auto bs = mapmanagerptr->getBlocks(path->getId());
                    if(!blockmanagerptr->tryAddBlockOccu(bs,getId(),path->getId())){
                        canResume = false;
                    }
                }
            }

            if(!canResume)continue;

            //could occu next station or path block?
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

            if (!blockmanagerptr->blockPassable(bs,getId())) {
                canResume = false;
                break;
            }
            if(canResume)
                resume();
        }
    }while(this->nowStation != endId || !isFinish());
    combined_logger->info("nowStation = {0}, endId = {1}", this->nowStation, endId);
}

void DyForklift::setQyhTcp(SessionPtr _qyhTcp)
{
    m_qTcp = _qyhTcp;
    if(m_qTcp==nullptr){
        status = Agv::AGV_STATUS_UNCONNECT;
    }
}
//发送消息给小车
bool DyForklift::send(const std::string &data)
{
    std::string sendContent = transToFullMsg(data);

    if(FORKLIFT_HEART != stringToInt(sendContent.substr(11, 2)))
    {
        combined_logger->info("send to agv{0}:{1}", id, sendContent);
    }
    if(nullptr == m_qTcp)
    {
        combined_logger->info("tcp is not available");
        return false;
    }
    bool res = m_qTcp->doSend(sendContent.c_str(), sendContent.length());
    if(!res){
        combined_logger->info("send to agv msg fail!");
    }
    if(sendContent.length()<13)return res;
    DyMsg msg;
    msg.msg = sendContent;
    msg.waitTime = 0;
    msgMtx.lock();
    m_unRecvSend[stoi(msg.msg.substr(1,6))] = msg;
    msgMtx.unlock();
    int msgType = stringToInt(msg.msg.substr(11,2));
    if(FORKLIFT_STARTREPORT != msgType && FORKLIFT_HEART != msgType)
    {
        m_unFinishCmd[msgType]= msg;
    }
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
    return resend(body.str());
}

//结束上报
bool DyForklift::endReport()
{
    std::stringstream body;
    body<<FORKLIFT_ENDREPORT;
    //eg:*123456001222#
    return resend(body.str());
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


bool DyForklift::charge(int params)
{
    std::stringstream body;
    body<<FORKLIFT_CHARGE;
    body<<params;
    return resend(body.str());
}

bool DyForklift::setInitPos(int station)
{
    MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
    if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)return false;

    MapPoint *point = static_cast<MapPoint *>(spirit);

    setPosition(0, station, 0);
    //占据初始位置
    MapManager::getInstance()->addOccuStation(station, shared_from_this());
    std::stringstream body;
    body<<FORKLIFT_INITPOS;
    body<<point->getRealX()/100.0<<","<<-point->getRealY()/100.0<<","<<point->getRealA()/10.0/57.3<<","<<MapManager::getInstance()->getFloor(station);
    return resend(body.str());
}


bool DyForklift::stopEmergency(int params)
{
    std::stringstream body;
    body<<FORKLIFT_STOP;
    body<<params;
    return resend(body.str());
}


bool DyForklift::isNeedTakeElevator(std::vector<int> lines)
{
    int endId = 0;
    combined_logger->info("DyForklift, isNeedTakeElevator");
    bool needElevator = false;

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        endId = path->getEnd();


        MapSpirit *pointSpirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *station = static_cast<MapPoint *>(pointSpirit);
        if (station == nullptr || station->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            combined_logger->error("DyForklift, station is null or not Map_Sprite_Type_Point!!!!!!!");
            continue;
        }

        if(station->getMapChange())
        {
            needElevator = true;
            break;
        }
    }
    return needElevator;
}

std::vector<int> DyForklift::getPathToElevator(std::vector<int> lines)
{
    std::vector<int> paths;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathToElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;


        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        if(end_point->getMapChange())
        {
            return paths;
        }
        paths.push_back(line);

    }
    return paths;
}

std::vector<int> DyForklift::getPathLeaveElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;
    bool elein = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if(addPath)
            paths.push_back(line);

        if(start_point->getMapChange() && end_point->getMapChange())
        {
            elein = true;
        }
        if(elein &&  !(start_point->getMapChange() && end_point->getMapChange()))
        {
            addPath = true;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathInElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathInElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if(start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
        }
        else if(paths.size())
        {
            return paths;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathEnterElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathEnterElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if(!start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathOutElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathOutElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if(start_point->getMapChange() && !end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

bool DyForklift::pause()
{
    combined_logger->debug("==============agv:{0} paused!",getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIST_NOLASER_PAUSE;
    sendPause = true;
    return resend(body.str());
}

bool DyForklift::resume()
{
    combined_logger->debug("==============agv:{0} resume!",getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIFT_NOLASER_RESUME;
    sendPause = false;
    return resend(body.str());
}

void DyForklift::onTaskCanceled(AgvTaskPtr _task)
{
    combined_logger->debug("==============agv:{0} task cancel! clear path!",getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIFT_NOLASER_CLEAR_TASK;
    resend(body.str());

    //release the end station occu and lines occs
    auto mapmanagerptr = MapManager::getInstance();
    if(currentTask!=nullptr){
        auto nodes = currentTask->getTaskNodes();
        auto index = currentTask->getDoingIndex();
        if(index<nodes.size())
        {
            auto node = nodes[index];
            mapmanagerptr->freeStation(node->getStation(),shared_from_this());
            auto paths = currentTask->getPath();
            for(auto p:paths){
                mapmanagerptr->freeLine(p,shared_from_this());
            }
        }
    }

    //TODO:
    //occu current station or  current path
}

