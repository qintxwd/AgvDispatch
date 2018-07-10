#ifndef QUNCHUANGNODETINGPUT_H
#define QUNCHUANGNODETINGPUT_H

#include "../agvtasknodedothing.h"

class QunChuangNodeTingPut : public AgvTaskNodeDoThing
{
public:
    QunChuangNodeTingPut(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 2 };

    int type(){return Type;}

    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("Qunchuang Agv PUT_GOOD");
    }

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

#endif // QUNCHUANGNODETINGPUT_H
