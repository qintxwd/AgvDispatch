#ifndef QUNCHUANGTCSCONNECTION_H
#define QUNCHUANGTCSCONNECTION_H

#include <string>
#include <memory>
#include <tuple>
#include "msg.h"
class QyhTcpClient;

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

    void taskStart(std::string dispatchId, int agvId);
    void taskFinished(std::string dispatchId, int agvId, bool success);



private:
    struct RequestContext;
    std::unique_ptr<RequestContext> requestContext; 

    QyhTcpClient *tcpClient;
    std::string ip;
    int port;

    std::vector<lynx::Msg> tcsTasks;
};

#endif // QUNCHUANGTCSCONNECTION_H
