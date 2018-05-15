#ifndef JACKINGAGVTHINGTURN_H
#define JACKINGAGVTHINGTURN_H

#include "../agvtasknodedothing.h"
//顶升agv 旋转,参数例如
//wait ms//超时时间2000//2秒超时
//params 发给agv的参数，例如 left 90//左转90度
//或者 right 90
class JackingAgvThingTurn : public AgvTaskNodeDoThing
{
public:
    JackingAgvThingTurn(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 1 };

    int type(){return Type;}

    enum{
        jack_error_type_send_fail = 0,
        jack_error_type_recv_fail,
        jack_error_type_recv_time_out,
        jack_error_type_excute_fail,
    };

    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("顶升AGV 旋转");
    }



private:
    int waitMs;
    bool left;
    int angle;
    bool bresult;//执行结果
};

#endif // JACKINGAGVTHINGTURN_H
