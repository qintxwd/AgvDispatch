#ifndef AGVMANAGER_H
#define AGVMANAGER_H

#include <vector>
#include <memory>
#include <mutex>
#include "utils/noncopyable.h"
#include "protocol.h"
#include "network/tcpsession.h"

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

    void addAgv(AgvPtr agv);

    void updateAgv(int id,std::string name,std::string ip,int port);

    void removeAgv(AgvPtr agv);

    void removeAgv(int agvId);

    void foreachAgv(AgvEachCallback cb);

    //用户接口
    void interList(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interAdd(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interDelete(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interModify(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
protected:
    AgvManager();
private:
    void checkTable();

    std::mutex mtx;
    std::vector<AgvPtr> agvs;
};

#endif // AGVMANAGER_H
