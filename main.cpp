#include "taskmanager.h"
#include "taskmaker.h"
#include "mapmap/mapmanager.h"
#include "agvmanager.h"
#include "usermanager.h"
#include "network/sessionmanager.h"
#include "msgprocess.h"
#include "userlogmanager.h"
#include "utils/Log/spdlog/spdlog.h"
#include "common.h"
#include "agvImpl/ros/agv/rosAgv.h"
#include "qunchuang/chipmounter/chipmounter.h"
#include "device/elevator/elevator.h"

#include "network/tcpclient.h"

//#define DY_TEST
void initLog()
{
    //日志文件
    try
    {
        std::vector<spdlog::sink_ptr> sinks;

        //控制台
#ifdef WIN32
        auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
        auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
        sinks.push_back(color_sink);

        //日志文件
        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt> ("agv_dispatch.log",  1024*1024*20, 5);
        sinks.push_back(rotating);

        combined_logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
        combined_logger->flush_on(spdlog::level::trace);
        combined_logger->set_level(spdlog::level::trace);

        //flush interval
        //        int q_size = 2000;
        //        spdlog::set_async_mode(q_size, spdlog::async_overflow_policy::block_retry,
        //                               nullptr,
        //                               std::chrono::seconds(1));

        ////test
        //combined_logger->info("=============log test==================");
        //combined_logger->trace("test log 1");
        //combined_logger->debug("test log 2");
        //combined_logger->info("test log 3");
        //combined_logger->warn("test log 4");
        //combined_logger->error("test log 5");
        //combined_logger->critical("test log 6");
        //combined_logger->info("=============log test==================");
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
}


void testAGV()
{
    g_threadPool.enqueue([&] {
        // test ros agv
        //rosAgvPtr agv(new rosAgv(1,"robot_0","192.168.8.206",7070));
        rosAgvPtr agv(new rosAgv(1,"robot_0","192.168.8.211",7070));

        std::cout << "AGV init...." << std::endl;

        agv->init();
        chipmounter *chip = new chipmounter(1,"chipmounter","10.63.39.190",1000);
        //chipmounter *chip = new chipmounter(1,"chipmounter","192.168.8.101",1024);
        chip->init();
        agv->setChipMounter(chip);

        sleep(20);

        chipinfo info;
        while(chip != nullptr)
        {
            if(!chip->isConnected())
            {
                //chip->init();
                //std::cout << "chipmounter disconnected, need reconnect...."<< std::endl;
            }

            if(chip->getAction(&info))
            {
                std::cout << "new task ...." << "action: " <<info.action<< "point: " <<info.point<< std::endl;
                if(agv->isAGVInit())
                {
                    chip->deleteHeadAction();
                    agv->startTask(info.point, info.action);
                    //agv->startTask( "2510", "loading");

                }
            }
            /*else
        {
            std::cout << "new task for test...." << "action: " <<info.action<< "point: " <<info.point<< std::endl;

            agv->startTask( "2511", "unloading");
            //agv->startTask( "", "");
            break;
        }*/

            sleep(1);
        }
    });


    std::cout << "testAGV end...." << std::endl;

}

/*void testElevator()
{
    // test elevator
    Elevator ele(1, "ele_0", "127.0.0.1", 8889);
    ele.init();
    g_threadPool.enqueue([&](){
        while (!ele.IsConnected())
            std::this_thread::sleep_for(std::chrono::microseconds(30));
        int from = 1;
        int to   = 2;
        int agv  = 1;
        // 请求某电梯 (30s超时)
        int elevator = ele.RequestTakeElevator(from, to, 0, agv, 30);
        if (elevator != -1) {
            // 乘梯应答
            ele.TakeEleAck(from, to, elevator, agv);
            // 等待电梯的进入指令 (30s超时)
            if (ele.ConfirmEleInfo(from, to, elevator, agv, 30)) {
                // todo: 此时agv可以进入, 进入过程每5秒发送一次乘梯应答
                //
                // 直到完全进入, agv发送进入电梯应答, 电梯开始运行直到到达目标楼层
                if (ele.AgvEnterUntilArrive(from, to, elevator, agv, 30)) {
                    // todo: 此时agv可以离开, 离开过程每5秒发送一次离开指令
                    //
                    // 直到完全离开, 发送离开应答结束乘梯流程
                    ele.AgvLeft(from, to, elevator, agv, 30);
                    return true;
                }
            }
            //
        }

        return false;
    });
}*/

int main(int argc, char *argv[])
{
    std::cout<<"start server ..."<<std::endl;

    //0.日志输出
    initLog();

    //1.打开数据库
    try{
        g_db.open(DB_File);
    }catch(CppSQLite3Exception &e){
        combined_logger->error("{0}:{1};",e.errorCode(),e.errorMessage());
        return -1;
    }

    //2.载入地图
    if(!MapManager::getInstance()->load()){
        combined_logger->error("map manager load fail");
        return -2;
    }

    //3.初始化车辆及其链接
    if(!AgvManager::getInstance()->init()){
        combined_logger->error("AgvManager init fail");
        return -3;
    }

    //4.初始化任务管理
    if(!TaskManager::getInstance()->init()){
        combined_logger->error("TaskManager init fail");
        return -4;
    }

    //5.用户管理
    UserManager::getInstance()->init();

    //6.初始化消息处理
    if(!MsgProcess::getInstance()->init()){
        combined_logger->error("MsgProcess init fail");
        return -5;
    }

    //7.初始化日志发布
    UserLogManager::getInstance()->init();

    //testAGV();//test ROS AGV, this only for test

    //8.初始化任务生成
    TaskMaker::getInstance()->init();

    //8.初始化tcp/ip 接口
    auto aID = SessionManager::getInstance()->addTcpAccepter(9999);
    SessionManager::getInstance()->openTcpAccepter(aID);

    //9.初始化websocket接口
    aID = SessionManager::getInstance()->addWebSocketAccepter(9998);
    SessionManager::getInstance()->openWebSocketAccepter(aID);
#ifdef DY_TEST
    aID = SessionManager::getInstance()->addAccepter(6789);
    SessionManager::getInstance()->openAccepter(aID);
#endif
    combined_logger->info("server init OK!");

    while (true) {
        sleep(1);
    }

    spdlog::drop_all();


    return 0;
}
