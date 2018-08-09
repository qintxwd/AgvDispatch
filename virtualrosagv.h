#ifndef VIRTUALROSAGV_H
#define VIRTUALROSAGV_H

#include "virtualagv.h"

class VirtualRosAgv : public VirtualAgv
{
public:
    VirtualRosAgv(int id,std::string name);

    ~VirtualRosAgv();

    enum { Type = Agv::Type +2 };

    virtual int type(){
        return Type;
    }

    virtual void init();

    virtual void excutePath(std::vector<int> lines);

    virtual void cancelTask();

    virtual void goStation(int station, bool stop = false);

    virtual void stop();

    virtual void callMapChange(int station);

    virtual void onTaskStart(AgvTaskPtr _task);

    virtual bool pause();

    virtual bool resume();

private:
    bool isStop;
    int lastStationOdometer;
    bool isPaused;
};

#endif // VIRTUALROSAGV_H
