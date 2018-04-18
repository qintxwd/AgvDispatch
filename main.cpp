#include "taskmanager.h"
#include "mapmanager.h"
#include "agvmanager.h"
#include "network/sessionmanager.h"

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
    //0.日志输出
    initLog();

    //1.载入地图
    if(!MapManager::getInstance()->load())
        return -1;

    //2.初始化车辆及其链接
    if(!AgvManager::getInstance()->init())
        return -2;

    //3.初始化任务管理
    if(!TaskManager::getInstance()->init())
        return -3;

    //4.初始化tcp/ip接口
    qyhnetwork::SessionManager::getRef().start();

    auto OnSessionBlock = [](qyhnetwork::TcpSessionPtr session, const char * begin, unsigned int len)
    {
        std::string content(begin, len);
        LOG(WARNING)<<"------------------------------------------------------------"
             "recv SessionID = " << session->getSessionID() << ", content = " << content;
        content += " ==> echo.";
        qyhnetwork::SessionManager::getRef().sendSessionData(session->getSessionID(), begin, len);

        //        //! step 3 stop server after 1 second.
        //        qyhnetwork::SessionManager::getRef().createTimer(1000, [](){
        //            qyhnetwork::SessionManager::getRef().stopAccept();
        //            qyhnetwork::SessionManager::getRef().kickClientSession();
        //            qyhnetwork::SessionManager::getRef().kickConnect();
        //            qyhnetwork::SessionManager::getRef().stop(); });
    };

    //TODO:
    auto aID = qyhnetwork::SessionManager::getRef().addAccepter("0.0.0.0", 9999);
    qyhnetwork::SessionManager::getRef().getAccepterOptions(aID)._sessionOptions._onBlockDispatch = OnSessionBlock;
    qyhnetwork::SessionManager::getRef().getAccepterOptions(aID)._setReuse = true;
    qyhnetwork::SessionManager::getRef().openAccepter(aID);

    //! step 2 running
    qyhnetwork::SessionManager::getRef().run();


    return 0;
}
