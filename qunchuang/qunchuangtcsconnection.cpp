#include "qunchuangtcsconnection.h"
#include "../network/tcpclient.h"
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
    static QunChuangTcsConnection tcs_conn("10.63.155.12", 2000);
    //static QunChuangTcsConnection tcs_conn("127.0.0.1", 2000);
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
    TcpClient::QyhClientReadCallback onread = std::bind(&QunChuangTcsConnection::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::QyhClientConnectCallback onconnect = std::bind(&QunChuangTcsConnection::onConnect, this);
    TcpClient::QyhClientDisconnectCallback ondisconnect = std::bind(&QunChuangTcsConnection::onDisconnect, this);
    tcpClient = new TcpClient(ip, port, onread, onconnect, ondisconnect);
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

lynx::Msg QunChuangTcsConnection::getTcsMsgByDispatchId(std::string dispatchId)
{
    lynx::Msg msg_data;
    for(int i=0; i<tcsTasks.size();i++)
    {
        msg_data = tcsTasks.at(i);
        if(dispatchId == msg_data["DISPATCH_ID"].string_value())
            return tcsTasks.at(i);
    }
    return nullptr;
}


lynx::Msg QunChuangTcsConnection::deleteTcsMsgByDispatchId(std::string dispatchId)
{
    lynx::Msg msg_data;
    for(int i=0; i<tcsTasks.size();i++)
    {
        msg_data = tcsTasks.at(i);
        if(dispatchId == msg_data["DISPATCH_ID"].string_value())
        {
            tcsTasks.erase(tcsTasks.begin()+i);
            break;
        }
    }
    return nullptr;
}

void QunChuangTcsConnection::onRead(const char *data, int len)
{
//#define MAX_MSG_LEN     (1024*10)
    std::string msg(data, (size_t)len);
    //msg = "S1F11L[13]<A[1] CEID ID='3'><A[10] LINE_ID ID='F1-A'><A[20] AGV_ID ID='1'><A[40] DISPATCH_ID ID='201806120010'><A[2] SEQUENCE ID='1'><A[20] FROM ID='2001'><A[20] DESTINATION ID='2510'><A[40] TRANSFER_OBJ_ID ID=''><A[40] CARRIER_ID ID=''><A[10] SETTING_CODE ID=''><A[5] SLOT_CNT ID='3'><A[20] MTRL_ID ID=''>L[3]L[2]<A[3] SLOTNO ID='1'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='1'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='1'><A[3] PRODUCT_CNT ID=''>";
    //msg = "S1F11L[13]<A[1] CEID ID='3'><A[10] LINE_ID ID='F1-A'><A[20] AGV_ID ID=''><A[40] DISPATCH_ID ID='201806120010'><A[2] SEQUENCE ID='1'><A[20] FROM ID='2510'><A[20] DESTINATION ID='2001'><A[40] TRANSFER_OBJ_ID ID=''><A[40] CARRIER_ID ID=''><A[10] SETTING_CODE ID=''><A[5] SLOT_CNT ID='3'><A[20] MTRL_ID ID=''>L[3]L[2]<A[3] SLOTNO ID='0'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='0'><A[3] PRODUCT_CNT ID=''>L[2]<A[3] SLOTNO ID='0'><A[3] PRODUCT_CNT ID=''>";

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
        //Add new task to tcsTasks
        tcsTasks.push_back(msg_data);

        //3层升降货架从下往上为第1层, 第2层, 第3层
        int firstFloorInfo = 0; //为3层升降货架第1层是否有料信息, 1为有料, 0为无料
        int secondFloorInfo = 0; //为3层升降货架第2层是否有料信息, 1为有料, 0为无料
        int thirdFloorInfo = 0; //为3层升降货架第3层是否有料信息, 1为有料, 0为无料


        std::string from    = msg_data["FROM"].string_value();
        //std::string from    = "2001";
        std::string to      = msg_data["DESTINATION"].string_value();
        //std::string to    = "2000";

        int ceid            = std::stoi(msg_data["CEID"].string_value());
        std::string dispatch_id  = msg_data["DISPATCH_ID"].string_value();

        std::string agv_id_str  = msg_data["AGV_ID"].string_value();
        int agv_id = -1;
        try
        {
            agv_id = std::stoi(agv_id_str);
        }
        catch(std::exception e)
        {

        }

        std::string line_id = msg_data["LINE_ID"].string_value();

        int arraySize = msg_data.array_items().size();
        combined_logger->info(" new TCS data,  size: {0}", arraySize);

        if(ceid == 1)
        {
            combined_logger->info(" 從上料點到下料點, [舊版]");
        }
        else if(ceid == 2)
        {
            combined_logger->info(" 從上料點到下料點, [適用于一般的AGV]");
        }
        else if(ceid == 3)
        {
            combined_logger->info(" 從上料點到下料點, [適用于多Port背負AGV]");
            if(arraySize == 13)
            {
                std::string str = msg_data[12][0]["SLOTNO"].string_value();
                if(str=="0" || str=="1")
                    firstFloorInfo = std::stoi(str);

                str = msg_data[12][1]["SLOTNO"].string_value();
                if(str=="0" || str=="1")
                    secondFloorInfo = std::stoi(str);

                str = msg_data[12][2]["SLOTNO"].string_value();
                if(str=="0" || str=="1")
                    thirdFloorInfo = std::stoi(str);

                std::cout<< " 3层升降货架第1层是否有料信息 : " << firstFloorInfo << std::endl;
                std::cout<< " 3层升降货架第2层是否有料信息 : " << secondFloorInfo << std::endl;
                std::cout<< " 3层升降货架第3层是否有料信息 : " << thirdFloorInfo << std::endl;
            }
            else
            {
                combined_logger->error("TCS data error,  dispatch_id: {1}", dispatch_id);
                return;
            }
        }


        combined_logger->info(" QunChuang new task, from : " + from);
        combined_logger->info("                       ceid :{0} ", ceid);
        combined_logger->info("                       to : " + to);
        combined_logger->info("              dispatch_id : " + dispatch_id);
        combined_logger->info("                   agv_id : " + agv_id_str);
        combined_logger->info("                  line_id : " + line_id);


        if(from.length() > 0)
            from = "station_"+from;

        if(to.length() > 0)
            to = "station_"+to;


        int allFloorInfo = thirdFloorInfo*4 + secondFloorInfo*2 + firstFloorInfo;

        // 返回信息
        lynx::Msg ret_msg({
                                  lynx::Msg(Msg::object{1, "RETURN_CODE", lynx::Msg("0")}),
                                  lynx::Msg(Msg::object{40, "DISPATCH_ID", msg_data["DISPATCH_ID"]}),
                                  lynx::Msg(Msg::object{2, "SEQUENCE", msg_data["SEQUENCE"]}),
                          });
        auto ret_msg_str = "S1F12" + ret_msg.dump();

        std::cout<<"@@@@@@@@ QunChuangTCS get new task, send data: " << ret_msg_str << std::endl;

        send(const_cast<char *>(ret_msg_str.c_str()), (int)ret_msg_str.size());

        //TODO
        TaskMaker::getInstance()->makeTask(from,to,dispatch_id,ceid,line_id,agv_id, allFloorInfo);


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


//S1F67, 啟動指令
void QunChuangTcsConnection::taskStart(std::string dispatchId, int agvId)
{
     combined_logger->info(" QunChuangTCS taskStart...... ");

     lynx::Msg msg_data=getTcsMsgByDispatchId(dispatchId);

     //if(nullptr != msg_data)
     {
         // 返回信息
         lynx::Msg ret_msg({
                                   lynx::Msg(Msg::object{1, "CEID", msg_data["CEID"]}),
                                   lynx::Msg(Msg::object{20, "LINE_ID", msg_data["LINE_ID"]}),
                                   lynx::Msg(Msg::object{20, "AGV_ID", lynx::Msg(intToString(agvId))}),
                                   lynx::Msg(Msg::object{40, "DISPATCH_ID", msg_data["DISPATCH_ID"]}),
                                   lynx::Msg(Msg::object{2, "SEQUENCE", lynx::Msg("0")}),
                                   lynx::Msg(Msg::object{20, "FROM", msg_data["FROM"]}),
                                   lynx::Msg(Msg::object{20, "DESTINATION", msg_data["DESTINATION"]}),
                                   lynx::Msg(Msg::object{40, "TRANSFER_OBJ_ID", msg_data["TRANSFER_OBJ_ID"]}),
                           });

         auto ret_msg_str = "S1F67" + ret_msg.dump();

         std::cout<<"@@@@@@@@ QunChuangTCS taskStart, send data: " << ret_msg_str << std::endl;

         send(const_cast<char *>(ret_msg_str.c_str()), (int)ret_msg_str.size());
     }
     /*else
     {
         combined_logger->error(" QunChuangTCS taskStart, msg_data = nullptr...... ");
     }*/
}


void QunChuangTcsConnection::taskFinished(std::string dispatchId, int agvId, bool success)
{
    combined_logger->info(" QunChuangTCS finished...... ");

    std::string dateTime = getTimeStrNow();

    lynx::Msg msg_data=getTcsMsgByDispatchId(dispatchId);

    //if(msg_data != nullptr)
    {
        std::string destination = msg_data["DESTINATION"].string_value();
        if(destination == " ")
            destination = msg_data["FROM"].string_value();

        // 返回信息
        lynx::Msg ret_msg({
                              lynx::Msg(Msg::object{1, "CEID", msg_data["CEID"]}),
                              lynx::Msg(Msg::object{20, "LINE_ID", msg_data["LINE_ID"]}),
                              lynx::Msg(Msg::object{20, "AGV_ID", lynx::Msg(intToString(agvId))}),
                              lynx::Msg(Msg::object{40, "DISPATCH_ID", msg_data["DISPATCH_ID"]}),
                              lynx::Msg(Msg::object{2, "SEQUENCE", lynx::Msg("0")}),
                              lynx::Msg(Msg::object{20, "OPER_ID", msg_data["DESTINATION"]}),
                              lynx::Msg(Msg::object{40, "TRANSFER_OBJ_ID", msg_data["TRANSFER_OBJ_ID"]}),
                              lynx::Msg(Msg::object{4, "ENDCODE", lynx::Msg("PREN")}),
                              lynx::Msg(Msg::object{80, "ENDREASON", lynx::Msg("")}),
                              lynx::Msg(Msg::object{14, "DATETIME", lynx::Msg(dateTime)}),
                          });

        auto ret_msg_str = "S1F15" + ret_msg.dump();

        std::cout<<"@@@@@@@@ QunChuangTCS finished, send data: " << ret_msg_str << std::endl;

        send(const_cast<char *>(ret_msg_str.c_str()), (int)ret_msg_str.size());
        //delete dispatch id

        deleteTcsMsgByDispatchId(dispatchId);
    }
    /*else
    {
        combined_logger->error(" QunChuangTCS finished, msg_data = nullptr...... ");
    }*/
}

