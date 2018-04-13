#ifndef AGVTASKNODEDOTHINGMANAGER_H
#define AGVTASKNODEDOTHINGMANAGER_H

#include <vector>
#include <string>

class AgvTaskNodeDoThingManager
{
public:


    static AgvTaskNodeDoThingManager* getInstance(){
        return p;
    }

    void init();

protected:
    AgvTaskNodeDoThingManager();
private:
    static AgvTaskNodeDoThingManager* p;
    std::vector<std::string> AgvTaskNodeDoThingManager::listDynamicLib();
};

#endif // AGVTASKNODEDOTHINGMANAGER_H
