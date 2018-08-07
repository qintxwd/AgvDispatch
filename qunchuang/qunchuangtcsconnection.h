#ifndef QUNCHUANGTCSCONNECTION_H
#define QUNCHUANGTCSCONNECTION_H

#include <string>
#include <memory>
#include <tuple>
#include <mutex>
#include "msg.h"
class TcpClient;

class QunChuangTcsConnection
{
public:
    typedef std::tuple<int, int, lynx::Msg> MsgTuple;
    static QunChuangTcsConnection *Instance();

    QunChuangTcsConnection(std::string ip, int port);
    ~QunChuangTcsConnection();
    void init();

    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();
    bool send(const char *data,int len);
    std::shared_ptr<MsgTuple> request(const MsgTuple& msg_tuple, int timeout);
    lynx::Msg getTcsMsgByDispatchId(std::string dispatchId);
    lynx::Msg deleteTcsMsgByDispatchId(std::string dispatchId);
    bool addTcsTask(lynx::Msg msg);
    bool isTcsTaskDuplicate(lynx::Msg msg_data, lynx::Msg msg);
    bool isTcsMsgExist(std::string dispatchId);


    void taskStart(std::string dispatchId, int agvId);
    void taskFinished(std::string dispatchId, int agvId, bool success);
    void reportArrivedStation(std::string dispatch_id, int agvId, std::string station_name);
    void reportLeaveStation(std::string dispatch_id, int agvId, std::string station_name);
    void processRequest(int S, int F,lynx::Msg);

    lynx::Msg createTCSReturnMsg(std::string return_code, lynx::Msg msg_data);


private:
    struct RequestContext;
    std::unique_ptr<RequestContext> requestContext; 

    TcpClient *tcpClient;
    std::string ip;
    int port;

    std::vector<lynx::Msg> tcsTasks;
    std::mutex tcsTasksMtx;

};

#endif // QUNCHUANGTCSCONNECTION_H
