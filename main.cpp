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
//#include "agvImpl/ros/agv/rosAgv.h"

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

    // test ros agv
    //rosAgvPtr agv(new rosAgv(1,"robot_0","127.0.0.1",7070));
    //agv->init();

    //8.初始化任务生成
    TaskMaker::getInstance()->init();

    //8.初始化tcp/ip 接口
    qyhnetwork::SessionManager::getInstance()->start();
    auto aID = qyhnetwork::SessionManager::getInstance()->addAccepter("0.0.0.0", 9999);
    qyhnetwork::SessionManager::getInstance()->getAccepterOptions(aID)._setReuse = true;
    qyhnetwork::SessionManager::getInstance()->openAccepter(aID);

    combined_logger->info("server init OK!");
    qyhnetwork::SessionManager::getInstance()->run();

    spdlog::drop_all();
    return 0;
}
