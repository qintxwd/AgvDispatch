#ifndef AGVTASKNODEDOTHING_H
#define AGVTASKNODEDOTHING_H

#include <memory>
#include <vector>
#include <string>

#include "agv.h"
//agv到达一个地方后要执行的多个事情中的一个
//做成插件模式

class AgvTaskNodeDoThing;
using AgvTaskNodeDoThingPtr = std::shared_ptr<AgvTaskNodeDoThing>;

class AgvTaskNodeDoThing : public std::enable_shared_from_this<AgvTaskNodeDoThing>
{
public:
    AgvTaskNodeDoThing(std::vector<std::string> _params):
        params(_params)
    {
    }

    virtual ~AgvTaskNodeDoThing(){
    }

    enum { Type = 0 };

    virtual int type(){return Type;}


    //4个虚函数，由基类去实现
    virtual void beforeDoing(AgvPtr agv) = 0;
    virtual void doing(AgvPtr agv) = 0;
    virtual void afterDoing(AgvPtr agv) = 0;
    virtual bool result(AgvPtr agv) = 0;

    virtual std::string discribe()  = 0;//用于保存数据库//简单描述以下干什么的

	std::vector<std::string> getParams() { return params; }

    void setStationId(int station_id)//MapSpirit id
    {
        m_station_id = station_id;
    }

    int getStationId()
    {
        return m_station_id;//MapSpirit id
    }
protected:
    std::vector<std::string> params;
    int m_station_id;//MapSpirit id
};

#endif // AGVTASKNODEDOTHING_H
