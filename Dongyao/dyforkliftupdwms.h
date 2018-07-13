#ifndef DYFORKLIFTUPDWMS_H
#define DYFORKLIFTUPDWMS_H

#include "../agvtasknodedothing.h"

class DyForkliftUpdWMS : public AgvTaskNodeDoThing
{
public:
    DyForkliftUpdWMS(std::vector<std::string> _params);

    enum { Type = AgvTaskNodeDoThing::Type + 2 };

    int type(){return Type;}


    void beforeDoing(AgvPtr agv);
    void doing(AgvPtr agv);
    void afterDoing(AgvPtr agv);
    bool result(AgvPtr agv);

    std::string discribe(){
        return std::string("UPDATE WMS");
    }



private:
    std::string m_store_no;
    std::string m_storage_no;
    std::string m_key_part_no;
    int m_type;
    bool bresult;//执行结果
};

#endif // DyForkliftUpdWMS_H
