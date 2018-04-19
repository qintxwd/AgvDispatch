#include "taskmanager.h"
#include "mapmanager.h"
#include "agvmanager.h"
#include "network/sessionmanager.h"
#include "msgprocess.h"
#include "utils/Log/easylogging.h"
INITIALIZE_EASYLOGGINGPP

void initLog()
{
    //日志配置文件
    el::Configurations fileConf("log-conf.conf");
    el::Loggers::reconfigureAllLoggers(fileConf);

    //日志支持多线程
    el::Loggers::addFlag(el::LoggingFlag::MultiLoggerSupport); // Enables support for multiple loggers

    //日志终端颜色输出
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput ); // Enables support for multiple loggers
}

int main(int argc, char *argv[])
{
    LOG(INFO)<<"start server ...";
#ifndef _WIN32
    //! linux下需要屏蔽的一些信号
    signal( SIGHUP, SIG_IGN );
    signal( SIGALRM, SIG_IGN );
    signal( SIGPIPE, SIG_IGN );
    signal( SIGXCPU, SIG_IGN );
    signal( SIGXFSZ, SIG_IGN );
    signal( SIGPROF, SIG_IGN );
    signal( SIGVTALRM, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGCHLD, SIG_IGN);
#endif
    //0.日志输出
    initLog();

    //1.载入地图
    if(!MapManager::getInstance()->load()){
        LOG(FATAL)<<"map manager load fail";
        return -1;
    }

    //2.初始化车辆及其链接
    if(!AgvManager::getInstance()->init()){
        LOG(FATAL)<<"AgvManager init fail";
        return -2;
    }

    //3.初始化任务管理
    if(!TaskManager::getInstance()->init()){
        LOG(FATAL)<<"TaskManager init fail";
        return -3;
    }

    //4.初始化消息处理
    if(!MsgProcess::getInstance()->init()){
        LOG(FATAL)<<"MsgProcess init fail";
        return -4;
    }

    //5.初始化tcp/ip 接口
    qyhnetwork::SessionManager::getInstance()->start();
    auto aID = qyhnetwork::SessionManager::getInstance()->addAccepter("0.0.0.0", 9999);
    qyhnetwork::SessionManager::getInstance()->getAccepterOptions(aID)._setReuse = true;
    qyhnetwork::SessionManager::getInstance()->openAccepter(aID);
    qyhnetwork::SessionManager::getInstance()->run();

    return 0;
}
