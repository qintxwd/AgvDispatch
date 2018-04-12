#include "taskmanager.h"
#include "mapmanager.h"
#include "agvmanager.h"

int main(int argc, char *argv[])
{
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

    //TODO:


    return 0;
}
