#ifndef AGVTASKNODEDOTHING_H
#define AGVTASKNODEDOTHING_H

#include <memory>

#include "agv.h"
//agv到达一个地方后要执行的多个事情中的一个
//做成插件模式

class AgvTaskNodeDoThing;
using AgvTaskNodeDoThingPtr = std::shared_ptr<AgvTaskNodeDoThing>;

class AgvTaskNodeDoThing : public std::enable_shared_from_this<AgvTaskNodeDoThing>
{
public:

    AgvTaskNodeDoThing()
    {
    }

    virtual ~AgvTaskNodeDoThing(){
    }

    //三个虚函数，由基类去实现
    virtual void beforeDoing(AgvPtr agv) = 0;
    virtual void doing(AgvPtr agv) = 0;
    virtual void afterDoing(AgvPtr agv) = 0;

    virtual std::string discribe()  = 0;//用于保存数据库//简单描述以下干什么的

};

#endif // AGVTASKNODEDOTHING_H
