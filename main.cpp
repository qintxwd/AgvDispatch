#include "taskmanager.h"
#include "mapmap/mapmanager.h"
#include "agvmanager.h"
#include "usermanager.h"
#include "network/sessionmanager.h"
#include "msgprocess.h"
#include "userlogmanager.h"
#include "utils/Log/spdlog/spdlog.h"
#include "common.h"

void initLog()
{
    //日志文件
    try
    {

        //set format
        //spdlog::set_pattern("[%^+++%$] [%H:%M:%S %z] [thread %t] %v");


//        std::vector<spdlog::sink_ptr> sinks;

//        //console sink
//        auto stdout_sink = spdlog::sinks::stdout_sink_mt::instance();
//#ifdef WIN32
//        combined_logger = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
//#else
//        auto sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
//#endif
//        sinks.push_back(sink);

        combined_logger =
//        //log file sink
//        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt> ("agv_dispatch", 1024*1024*20, 5);
//        sinks.push_back(rotating);

        //combine
        //combined_logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));


        //spdlog::register_logger(combined_logger);
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
