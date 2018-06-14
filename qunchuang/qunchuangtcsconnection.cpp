#include "qunchuangtcsconnection.h"
#include "../qyhtcpclient.h"
#include "../taskmaker.h"
#include "../common.h"
#include <string.h>
#include <functional>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "msg.h"

using namespace lynx;

// 请求上下文,  用去同步send/recv
// 1. send线程发送请求, 记录请求的信息(S & F), 在recv线程中根据请求上下文
//     判断响应信息是否正确 (如果响应延时, 而send比较频繁会在覆盖之前的上下文)
// 2. send线程发送请求, 产生条件变量, 线程进入等待, 在recv线程得到响应后
//     通知所有等待的send线程, send线程根据条件判断是否是属于自己的响应,
//     唤醒,执行后续操作
struct QunChuangTcsConnection::RequestContext {
    std::mutex              mtx;
    std::condition_variable cv;

    int         S;
    int         F;
    lynx::Msg   msg;
};



QunChuangTcsConnection *QunChuangTcsConnection::Instance()
{
    static QunChuangTcsConnection tcs_conn("127.0.0.1", 8888);
    return &tcs_conn;
}

QunChuangTcsConnection::QunChuangTcsConnection(std::string _ip, int _port):
    ip(_ip),
    port(_port),
    tcpClient(nullptr),
    requestContext(new QunChuangTcsConnection::RequestContext)
{

}
QunChuangTcsConnection::~QunChuangTcsConnection(){
    if (tcpClient)delete tcpClient;
}

void QunChuangTcsConnection::init() {
    QyhTcpClient::QyhClientReadCallback onread = std::bind(&QunChuangTcsConnection::onRead, this, std::placeholders::_1, std::placeholders::_2);
    QyhTcpClient::QyhClientConnectCallback onconnect = std::bind(&QunChuangTcsConnection::onConnect, this);
    QyhTcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&QunChuangTcsConnection::onDisconnect, this);
    tcpClient = new QyhTcpClient(ip, port, onread, onconnect, ondisconnect);
}

bool QunChuangTcsConnection::send(const char *data,int len)
{
    if(tcpClient!=nullptr)
        return tcpClient->sendToServer(data,len);

    return false;
}

static std::string serailize(int s, int f, const lynx::Msg& msg) {
    std::ostringstream oss;
    oss << "S" << s << "F" << f << msg.dump();
    return oss.str();
}

// bool QunChuangTcsConnection::request(int S, int F, const lynx::Msg& msg,
//                     const std::function<bool (int, int, const lynx::Msg&)>& f);

std::shared_ptr<QunChuangTcsConnection::MsgTuple> 
QunChuangTcsConnection::request(const QunChuangTcsConnection::MsgTuple& msg_tuple, int ms)
{
    int S, F;
    lynx::Msg msg;

    std::tie(S, F, msg) = msg_tuple;
    std::string req = serailize(S, F, msg);
    combined_logger->debug("tcs request msg: {}", req);

    // 发送成功
    if (send(req.c_str(), (int)req.size())) {
        std::unique_lock<std::mutex> lck(requestContext->mtx);
        requestContext->cv.wait_for(lck, std::chrono::microseconds(ms));
        auto resp = serailize(requestContext->S, requestContext->F, requestContext->msg);
        combined_logger->debug("tcs response msg: {}", resp);
        return std::make_shared<QunChuangTcsConnection::MsgTuple>(
            requestContext->S, requestContext->F, requestContext->msg
        );
    }

    return nullptr;
}

void QunChuangTcsConnection::onRead(const char *data, int len)
{
//#define MAX_MSG_LEN     (1024*10)
    std::string msg(data, (size_t)len);
    msg = "S1F11L[13]<A[1] CEID ID='0010'><A[10] LINE_ID ID='F1-A'><A[20] AGV_ID ID=''><A[40] DISPATCH_ID ID='201806120010'><A[2] SEQUENCE ID='1'><A[20] FROM ID='0032'><A[20] DESTINATION ID='0034'><A[40] TRANSFER_OBJ_ID ID=''><A[40] CARRIER_ID ID=''><A[10] SETTING_CODE ID=''><A[5] SLOT_CNT ID='3'><A[20] MTRL_ID ID=''>L[3]L[2]<A[3] SLOTNO ID='1'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='2'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='3'><A[3] PRODUCT_CNT ID=''>";
    combined_logger->info("TCS RECV:"+msg);

    // 解析msg, SnFn<MSG>
    int S, F;
    lynx::Msg msg_data;
    std::string err;
    auto ret = lynx::parseMsg(msg, err);

    // 出错返回
    if (!err.empty()) {
        std::cerr << err << std::endl;
        return;
    }
    std::tie(S, F, msg_data) = ret;

    // S = 1 && F == 11
    if (S==1 && F==11) {
        std::string from    = msg_data["FROM"].string_value();
        std::string to      = msg_data["DESTINATION"].string_value();
        int ceid            = std::stoi(msg_data["CEID"].string_value());
        std::string dispatch_id  = msg_data["DISPATCH_ID"].string_value();


        combined_logger->info(" QunChuangTcsConnection,  makeTask");


        //TODO
        TaskMaker::getInstance()->makeTask(from,to,dispatch_id,ceid);

        // 返回信息
        lynx::Msg ret_msg({
                                  lynx::Msg(Msg::object{1, "RETURN_CODE", lynx::Msg("0")}),
                                  lynx::Msg(Msg::object{40, "DISPATCH_ID", msg_data["DISPATCH_ID"]}),
                                  lynx::Msg(Msg::object{2, "SEQUENCE", lynx::Msg("0")}),
                          });
        auto ret_msg_str = "S1F12" + ret_msg.dump();
        send(const_cast<char *>(ret_msg_str.c_str()), (int)ret_msg_str.size());
    }
    else {
        std::unique_lock<std::mutex> lck(requestContext->mtx);
        requestContext->S = S;
        requestContext->F = F;
        requestContext->msg = msg_data;
        requestContext->cv.notify_all();
    }
}

void QunChuangTcsConnection::onConnect()
{
    //TODO
}

void QunChuangTcsConnection::onDisconnect()
{
    //TODO
}
