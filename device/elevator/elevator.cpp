#include "elevator.h"
#include <chrono>
#include <future>
#include <atomic>            
#include <mutex>
#include <condition_variable>

static const std::string empty_string;

int Elevator::POSITION_NO_AGV = -1;
std::string Elevator::POSITION_LEFT = "left";
std::string Elevator::POSITION_RIGHT = "right";
std::string Elevator::POSITION_MIDDLE = "middle";

struct Elevator::ElevatorCtx {
    std::mutex              mtx;
    std::condition_variable cv;
    // 每个request 响应的内容
    std::string             err  = empty_string;
    lynx::elevator::Param   param;
    // 针对响应信息agv id是0的情况, 需要确认是否由agv已经处理
    std::atomic_bool        is_handled;
};

Elevator::Elevator(int _id, std::string _name, std::string _ip, int _port, bool _leftEnabled, bool _rightEnabled, bool _elevatorEnabled,std::string _waitingPoints)
    : Device(_id, _name, _ip,  _port)
    , ctx_(new Elevator::ElevatorCtx)
    , connected_(false)
{
    agv_num = 0;
    send_cmd = false;

    left_pos_enabled = _leftEnabled;  //启用电梯左侧位置
    right_pos_enabled = _rightEnabled; //启用电梯右侧位置
    elevator_enabled = _elevatorEnabled;  //启用电梯

    combined_logger->info("[Elevator id: {0}] [ip: {1}], [waitingPoints: {2}]",
                          _id,
                          _ip,
                          _waitingPoints);

    ElevatorPositon* p_middle = new ElevatorPositon(POSITION_MIDDLE, POSITION_NO_AGV, false, false);
    ElevatorPositon* p_left = new ElevatorPositon(POSITION_LEFT, POSITION_NO_AGV, false, false);
    ElevatorPositon* p_right = new ElevatorPositon(POSITION_RIGHT, POSITION_NO_AGV, false, false);

    //默认启用电梯中间位置
    if(elevator_enabled == true && left_pos_enabled == false && right_pos_enabled == false)
    {
        p_middle->setEnabled(true);
    }
    else if(elevator_enabled == true)
    {
        p_left->setEnabled(left_pos_enabled);
        p_right->setEnabled(right_pos_enabled);
    }

    std::vector<std::string> pvs = split(_waitingPoints, ";");
    for (auto p : pvs) {
        int intp;
        std::stringstream ss;
        ss << p;
        ss >> intp;

        std::cout<< " intp : " << intp << std::endl;

        waitingPoints.push_back(intp);
    }

    resetFlag = false;
}

Elevator::~Elevator()
{
    std::map<std::string,ElevatorPositon *>::iterator iter;//定义一个迭代指针iter

    for(iter=position_list_.begin(); iter!=position_list_.end(); iter++)
    {
        if(iter->second != nullptr)
            delete iter->second;
    }

}

// 单次请求
// p        -> 发送的请求
// cmd      -> 期望得到的响应
// timeout  -> 超时时间
// 返回响应内容
std::shared_ptr<Elevator::EleParam> Elevator::request(const Elevator::EleParam& p,
                                                      int cmd,
                                                      int timeout)
{
    combined_logger->info("[Elevator {0}] [{1}], Agv {2} Send Request: {3}"
                          " Expected Get Cmd {4}", name,
                          (connected_ ? "Connected" : "Disconnected"),
                          p.robot_no, p.debug(), cmd);
    if (!connected_) return nullptr;
    auto data = p.serialize();


    // send and recv (忽略发送延时)
    if (send((char *)(data.data()), data.size())) {
        std::unique_lock<std::mutex> lck(ctx_->mtx);

        auto pred = [&, p, cmd]() {
            if (ctx_->err == empty_string) {
                // 响应的agv id == 0, 表示询问, 任意agv都可以接收

                int agv_resp = (int)ctx_->param.robot_no;
                int ele_resp = (int)ctx_->param.elevator_no;
                bool ele_as_expect = (ele_resp == p.elevator_no);
                bool agv_as_expect = (agv_resp == p.robot_no);
                bool cmd_as_expect = (ctx_->param.cmd == cmd);

                if (agv_resp == 0) {
                    if (!ctx_->is_handled && cmd_as_expect && ele_as_expect) {
                        ctx_->is_handled = true;
                        return true;
                    }
                    return false;
                }
                else {
                    return agv_as_expect && cmd_as_expect;
                }
            }
            else {
                return false;
            }
        };

        auto res = ctx_->cv.wait_for(lck,
                                     std::chrono::seconds(timeout),
                                     pred);

        if (res) {
            combined_logger->info("[Elevator {0}] [{1}], Agv {2} Get Response",
                                  name,
                                  (connected_ ? "Connected" : "Disconnected"),
                                  p.robot_no);
            std::shared_ptr<lynx::elevator::Param> p(new lynx::elevator::Param);
            *p = ctx_->param;
            return p;
        }
        else {
            combined_logger->info("[Elevator {0}] [{1}], Agv {2} Request {3} Timeout"
                                  , name
                                  , (connected_ ? "Connected" : "Disconnected")
                                  , p.robot_no
                                  , p.cmd);
            return nullptr;
        }
    }

    return nullptr;
}

// 通知不必有信息返回
bool Elevator::notify(const lynx::elevator::Param& p)
{
    combined_logger->info("[Elevator {0}] [{1}], Agv {2} Send Notify: {3}",
                          name,
                          (connected_ ? "Connected" : "Disconnected"),
                          p.robot_no,
                          p.debug());
    if (!connected_) return false;

    auto&& data = p.serialize();
    return send((char *)(data.data()), data.size());
}

// 等待服务器命令
std::shared_ptr<lynx::elevator::Param> Elevator::waitfor(int agv_id, lynx::elevator::CMD cmd, int timeout)
{
    combined_logger->info("[Elevator {0}] [{1}], Agv {2} Waiting Cmd: {3}",
                          name,
                          (connected_ ? "Connected" : "Disconnected"),
                          agv_id,
                          cmd);
    if (!connected_) return nullptr;

    // 等待response
    std::unique_lock<std::mutex> lck(ctx_->mtx);
    auto pred = [&, agv_id, cmd]() {
        if (ctx_->err == empty_string) {
            // 响应的agv id == 0, 表示询问, 任意agv都可以接收
            int agv_resp = (int)ctx_->param.robot_no;
            bool cmd_as_expect = (ctx_->param.cmd == cmd);

            if (agv_resp == 0) {
                if (!ctx_->is_handled && cmd_as_expect) {
                    ctx_->is_handled = true;
                    return true;
                }
                return false;
            }
            else {
                bool agv_as_expect = (agv_resp == agv_id);
                return agv_as_expect && cmd_as_expect;
            }
        }
        else {
            return false;
        }
    };

    auto res = ctx_->cv.wait_for(lck,
                                 std::chrono::seconds(timeout),
                                 pred);


    // 超时
    if (!res) {
        combined_logger->info("[Elevator {0}] [{1}], Agv {2} Wait For Cmd {3} Timeout",
                              name,
                              (connected_ ? "Connected" : "Disconnected"),
                              agv_id, cmd);
        return nullptr;
    }
    else {
        std::shared_ptr<lynx::elevator::Param> p(new lynx::elevator::Param);
        *p = ctx_->param;
        combined_logger->info("[Elevator {0}] [{1}], Agv {2}  Got Cmd {3} ",
                              name,
                              (connected_ ? "Connected" : "Disconnected"),
                              agv_id,
                              cmd);
        return p;
    }
}

static lynx::elevator::Param create_param(int cmd, int from_floor, 
                                          int to_floor, int elevator_id, int agv_id)
{
    using namespace lynx::elevator;
    return lynx::elevator::Param {
        (Param::byte_t) from_floor,
                (Param::byte_t) to_floor,
                (CMD)           cmd,
                (Param::byte_t) elevator_id,
                (Param::byte_t) agv_id,
    };
}

/*请求乘电梯
 *from_floor: AGV 所在楼层
 *to_floor: 到达楼层
 *elevator_id: 电梯ID
 *agv_id: AGV ID
 *返回值 此函数会阻塞 直到返回请求, true 为接受请求门打开, false为拒绝
 *timeout: 秒, 超时返回
 */
int Elevator::RequestTakeElevator(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    using namespace lynx::elevator;


    std::shared_ptr<Param> resp = nullptr;
    do  {
        // 呼梯问询

        std::cout << "create_param....elevator_id: "<<elevator_id << std::endl;

        //auto p = create_param(CallEleENQ, from_floor, to_floor, elevator_id, agv_id);
        //向内呼发送不需要to_floor
        auto p = create_param(CallEleENQ, 0, from_floor, this->getId(), agv_id);

        auto resp_param = request(p, CallEleACK, timeout);
        if (resp_param == nullptr) {
            return -1;
        }

        resp = waitfor(agv_id, lynx::elevator::TakeEleENQ, timeout);
    } while(resp == nullptr);

    // 判断楼层是否相等, 是否乘梯
    // 响应消息中 目标楼层1表示电梯门开着, 0表示电梯门关闭
    int id = (resp->src_floor == from_floor && resp->dst_floor == 1 && resp->elevator_no == this->getId()) ?
                resp->elevator_no : -1;
    combined_logger->info("[Elevator {0}] [{1}], Request Elevator Succeed, Ele ID: {2}",
                          name, (connected_ ? "Connected" : "Disconnected"),
                          id);
    return id;
}

void Elevator::TakeEleAck(int from_floor, int to_floor, int elevator_id, int agv_id)
{
    auto p = create_param(lynx::elevator::TakeEleACK, from_floor, to_floor, elevator_id, agv_id);

    notify(p);
}

bool Elevator::ConfirmEleInfo(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    //TakeEleAck(from_floor, to_floor, elevator_id, agv_id);
    auto p = waitfor(agv_id, lynx::elevator::IntoEleENQ, timeout);

    auto tmp = create_param(lynx::elevator::IntoEleENQ, from_floor,
                            to_floor, elevator_id, agv_id);
    return p && *p == tmp;
}

bool Elevator::AgvEnterConfirm(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    // agv 发送进入电梯应答
    // ele 响应进入电梯确认
    // 如果失败退出
    std::shared_ptr<lynx::elevator::Param> resp;

    auto p = create_param(lynx::elevator::IntoEleACK, from_floor,
                          to_floor, elevator_id, agv_id);
    /*if (!request(p, lynx::elevator::IntoEleSet, timeout))
         return false;*/

    return notify(p);

}

bool Elevator::AgvWaitArrive(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    //等待电梯到达指令, 如果电梯未到达, need重新发送进入电梯应答
    std::shared_ptr<lynx::elevator::Param> resp;

    resp = waitfor(agv_id, lynx::elevator::LeftEleENQ, timeout);
    if(resp != nullptr)
        return true;
    else
        return false;
}

bool Elevator::AgvEnterUntilArrive(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    // agv 发送进入电梯应答
    // ele 响应进入电梯确认
    // 如果失败退出
    // 并等待电梯到达指令, 如果电梯未到达, 重新发送进入电梯应答
    std::shared_ptr<lynx::elevator::Param> resp;
    do {
        auto p = create_param(lynx::elevator::IntoEleACK, from_floor,
                              to_floor, elevator_id, agv_id);
        if (!request(p, lynx::elevator::IntoEleSet, timeout))
            return false;

        resp = waitfor(agv_id, lynx::elevator::LeftEleENQ, timeout);
    } while(resp == nullptr);

    return true;
}

bool Elevator::AgvStartLeft(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    auto p = create_param(lynx::elevator::LeftEleCMD, from_floor, to_floor,
                          elevator_id, agv_id);

    return nullptr !=
            request(p, lynx::elevator::LeftCMDSet, timeout);
}


bool Elevator::AgvLeft(int from_floor, int to_floor, int elevator_id, int agv_id, int timeout)
{
    auto p = create_param(lynx::elevator::LeftEleACK, from_floor, to_floor,
                          elevator_id, agv_id);

    return nullptr !=
            request(p, lynx::elevator::LeftEleSet, timeout);
}


void Elevator::StartSendThread(int cmd, int from_floor, int to_floor, int elevator_id, int agv_id)
{
    send_cmd = true;

    thread t = std::thread([&,cmd,from_floor,to_floor,elevator_id,agv_id](){
        do
        {
            combined_logger->info("Elevator 发送CMD :{0}", cmd);

            auto p = create_param(cmd, from_floor, to_floor, elevator_id, agv_id);
            if(cmd == lynx::elevator::TakeEleACK)
            {
                combined_logger->info("AGV发送乘梯应答....", cmd);
            }
            else if(cmd == lynx::elevator::LeftEleCMD)
            {
                combined_logger->info("AGV发送离开指令....", cmd);
            }

            notify(p);
            sleep(5);
            combined_logger->info("AGV发送CMD end", cmd);
        }
        while(send_cmd);
    });

    t.detach();
}

void Elevator::StopSendThread()
{
    send_cmd = false;
}

// 一些测试用函数
// 测试状态切换
void Elevator::SwitchElevatorState(int elevator_id)
{
    auto p = create_param(lynx::elevator::TestEleCmd,
                          0x00, 0x00, elevator_id, 0x00);
    notify(p);
}

// 复位电梯状态
bool Elevator::ResetElevatorState(int elevator_id)
{
    auto p = create_param(lynx::elevator::InitEleENQ,
                          0x00, 0x00, elevator_id, 0x00);
    //return nullptr != request(p, lynx::elevator::InitEleACK, 10);
    auto data = p.serialize();

    // send and recv (忽略发送延时)
    notify(p);
    std::cout << "复位电梯状态 success...." << std::endl;

}

// 电梯状态询问
std::shared_ptr<Elevator::EleParam> 
Elevator::GetElevatorState(int elevator_id)
{
    auto p = create_param(lynx::elevator::StaEleENQ,
                          0x00, 0x00, elevator_id, 0x00);
    return request(p, lynx::elevator::StaEleACK, 30);
}

// 内部通信测试
bool Elevator::PingElevator(int elevator_id)
{
    auto p = create_param(lynx::elevator::InComENQRobot,
                          0xFF, 0xFF, elevator_id, 0x0);
    return nullptr != request(p, lynx::elevator::InComENQServ, 30);
}


bool Elevator::OpenDoor(int floor)
{
    return false;
}

void Elevator::CloseDoor(int floor)
{

}

//保持电梯门打开
void Elevator::KeepingDoorOpen(int floor)
{

}

//电梯到达楼层
void Elevator::onElevatorArrived(int floor)
{

}


static inline std::string on_read_debug(const char* data, int len) {
    std::ostringstream oss;

    oss << "[";
    for (int i=0; i<len; i++) {
        oss << std::setfill('0') << std::setw(2)
            << std::setiosflags(std::ios::uppercase)
            << std::hex << (int)((unsigned char)data[i]);
        if (i != len-1) oss << " ";
    }
    oss << "]";
    return oss.str();
}

void Elevator::onRead(const char *data,int len)
{
    // parse
    std::string err;
    using namespace lynx::elevator;
    auto p = Param::parse((Param::byte_t *)data, len, err);

    // lock
    std::unique_lock<std::mutex> lock(ctx_->mtx);
    if (!err.empty()) {
        //解析出错, 跳过响应
        ctx_->err = err;
        combined_logger->error("[Elevator {0}] OnRead Source: {1} Error: {2}",
                               name,
                               on_read_debug(data, len),
                               err);
    }
    else {
        ctx_->err       = empty_string;
        ctx_->param     = p;
        combined_logger->info("[Elevator {0}] OnRead: {1}", name, p.debug());
        // 所有agv都可以接收, flag ( is_handled ) 用于限制只有一个可以接收处理
        if (p.robot_no == 0) {
            ctx_->is_handled = false;
        }
        ctx_->cv.notify_all();
    }
}

void Elevator::onConnect()
{
    connected_ = true;
    combined_logger->info("[Elevator {0}] Connected, 复位电梯状态", name);

    if(!resetFlag)
    {
        //防止重连后复位电梯
        ResetElevatorState(this->getId());
        resetFlag = true;
    }
}

void Elevator::onDisconnect()
{
    //combined_logger->warn("[Elevator {0}] Disconnected", name);
    connected_ = false;
}

void Elevator::reconnect()
{

}
