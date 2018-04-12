#ifndef AGVTASKNODEDOTHING_H
#define AGVTASKNODEDOTHING_H
#include "agv.h"
//agv到达一个地方后要执行的多个事情中的一个
class AgvTaskNodeDoThing
{
public:

    AgvTaskNodeDoThing()
    {
    }

    //三个虚函数，由基类去实现
    virtual void beforeDoing(Agv *agv) = 0;
    virtual void doing(Agv *agv) = 0;
    virtual void afterDoing(Agv *agv) = 0;

    virtual std::string discribe()  = 0;//用于保存数据库//简单描述以下干什么的

};

#endif // AGVTASKNODEDOTHING_H
