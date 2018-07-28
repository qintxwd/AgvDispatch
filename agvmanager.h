#ifndef AGVMANAGER_H
#define AGVMANAGER_H

#include <vector>
#include <memory>
#include <mutex>
#include "utils/noncopyable.h"
#include "protocol.h"
#include "network/session.h"

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

class AgvManager;
using AgvManagerPtr = std::shared_ptr<AgvManager>;

class AgvManager: public noncopyable,public std::enable_shared_from_this<AgvManager>
{
public:
    typedef std::function< void(AgvPtr) >  AgvEachCallback;

    bool init();

    static AgvManagerPtr getInstance(){
        static AgvManagerPtr m_ins = AgvManagerPtr(new AgvManager());
        return m_ins;
    }

    AgvPtr getAgvById(int id);

    AgvPtr getAgvByIP(std::string ip);

    void addAgv(AgvPtr agv);

    void updateAgv(int id,std::string name,std::string ip,int port,int lastStation, int nowStation, int nextStation);

    void removeAgv(AgvPtr agv);

    void removeAgv(int agvId);

    void foreachAgv(AgvEachCallback cb);

    void getPositionJson(Json::Value &json);

    void setServerAccepterID(int serverID);

    int getServerAccepterID(){return _serverID;}
    //用户接口
    void interList(SessionPtr conn, const Json::Value &request);
    void interAdd(SessionPtr conn, const Json::Value &request);
    void interDelete(SessionPtr conn, const Json::Value &request);
    void interModify(SessionPtr conn, const Json::Value &request);
protected:
    AgvManager();
private:
    void checkTable();

    std::mutex mtx;
    std::vector<AgvPtr> agvs;
    int _serverID;
};

#endif // AGVMANAGER_H
