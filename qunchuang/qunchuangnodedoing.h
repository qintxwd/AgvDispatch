#ifndef QUNCHUANGNODEDOING_H
#define QUNCHUANGNODEDOING_H

#include "../agvtasknodedothing.h"

class QunChuangNodeDoing : public AgvTaskNodeDoThing
{
public:
    QunChuangNodeDoing(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 1 };

    int type(){return Type;}

    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("Qunchuang Agv DOING");
    }

    /*
     * qunchuang staion like this "station_2500", this function remove "station_"
     */
    std::string getStationNum(std::string stationName)
    {
        std::string result;
        if (stationName.length() == 0)
            return result;

        size_t pos = stationName.find_first_of("_");
        if(pos == std::string::npos){
            return stationName;
        }

        result = stationName.substr(pos+1);
        return result;
    }

private:
    bool bresult;//执行结果
};

#endif //QUNCHUANGNODEDOING_H
