#ifndef AGVMANAGER_H
#define AGVMANAGER_H

#include <vector>
#include <mutex>
#include "utils/noncopyable.h"

class Agv;

class AgvManager: public noncopyable
{
public:
    typedef std::function< void(Agv*) >  AgvEachCallback;

    bool init();

    static AgvManager* getInstance(){
        return p;
    }

    void addAgv(Agv *agv);

    void removeAgv(Agv *agv);

    void removeAgv(int agvId);

    void foreachAgv(AgvEachCallback cb);
protected:
    AgvManager();
private:
    void checkTable();

    static AgvManager* p;

    std::mutex mtx;
    std::vector<Agv *> agvs;
};

#endif // AGVMANAGER_H
