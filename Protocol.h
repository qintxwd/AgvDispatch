#pragma once

#include <cstdint>

//define msg from client to server and msg from server to client
// make each msg length <= 1024

#define GLOBAL_SERVER_PORT   9999

#define ONE_MSG_MAX_LENGTH  (1024)              //一条消息的最大长度（请求消息:head+body  响应消息: head+return_head+body）

#define	MSG_REQUEST_BODY_MAX_SIZE           (1000)  //一条消息的内容部分最大长度
#define MSG_RESPONSE_BODY_MAX_SIZE          (768)   //一条响应消息的最大长度
#define MSG_LOG_MAX_LENGTH              (MSG_RESPONSE_BODY_MAX_SIZE - 24)
#define MSG_MSG_Head_HEAD		(0x88)
#define MSG_MSG_Head_TAIL		(0xAA)

#define MSG_TIME_STRING_LEN            (24)
#define MSG_SHORT_STRING_LEN           (32)
#define MSG_STRING_LEN                  (64)
#define MSG_LONG_STRING_LEN             (128)
#define MSG_LONG_LONG_STRING_LEN        (512)
#define SQL_MAX_LENGTH                  (1024)



#pragma pack(1)

//消息的head
typedef struct _MSG_Head
{
    uint8_t head;//固定为0x88  MSG_MSG_Head_HEAD
	uint32_t body_length;//body的长度 最大为TCP_MSG_MAX_SIZE
	uint8_t todo;//要做的事情
	uint32_t queuenumber;//消息序号，返回时要带上
    uint8_t flag;//标志位，默认为0，0:完成  1:未完成
    uint8_t tail;//固定为0xAA [head 和tail 决定了这个消息是否是 我们定义的消息]  MSG_MSG_Head_TAIL
}MSG_Head;

//!!!完整的请求消息
typedef struct _MSG_Request {
    MSG_Head head;
    char body[MSG_REQUEST_BODY_MAX_SIZE];
}MSG_Request;

//result位的定义
enum
{
    RETURN_MSG_RESULT_SUCCESS = 0,//全局的成功
    RETURN_MSG_RESULT_FAIL,//全局的错误
};
//error_code位的定义
enum {
    RETURN_MSG_ERROR_NO_ERROR = 0,
    RETURN_MSG_ERROR_CODE_UNKNOW,//未知错误
    RETURN_MSG_ERROR_CODE_LENGTH,//数据长度有问题
    RETURN_MSG_ERROR_CODE_PERMISSION_DENIED,//
    RETURN_MSG_ERROR_CODE_USERNAME_NOT_EXIST,//登陆用户名不存在
    RETURN_MSG_ERROR_CODE_PASSWORD_ERROR,//登陆密码错误
    RETURN_MSG_ERROR_CODE_NOT_LOGIN,//用户未登录
    RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL,
    RETURN_MSG_ERROR_CODE_SAVE_SQL_FAIL,//保存数据库失败
    RETURN_MSG_ERROR_CODE_TASKING,//有任务正在执行
    RETURN_MSG_ERROR_CODE_NOT_CTREATING,//不是正在创建地图的时候添加 站点啊、直线、曲线
    RETURN_MSG_ERROR_CODE_CTREATING,//正在创建地图的时候获取地图
	RETURN_MSG_ERROR_CODE_UNFINDED,//未找到
};
//返回消息的结构的额外头
typedef struct _MSG_RESPONSE_HEAD
{
    uint8_t result;//RETURN_MSG_RESULT
    uint32_t error_code;//RETURN_MSG_ERROR_CODE_
    char error_info[MSG_LONG_STRING_LEN];
}MSG_RESPONSE_HEAD;

//!!!完整的返回消息
typedef struct _MSG_Response {
    MSG_Head head;//先读取固定长度的head(11Byte) 然后又读取固定长度的return_head(261Byte) 最后根据head中的body_length,读取 body_length的 body缓存
    MSG_RESPONSE_HEAD return_head;//用于判断返回结果
    char body[MSG_RESPONSE_BODY_MAX_SIZE];//content
}MSG_Response;
//////////////////////////下面定义具体的消息协议

//定义消息头的 todo
typedef enum Msg_Todo
{
    //request and response
    MSG_TODO_USER_LOGIN = 0,//登录//username[MSG_STRING_LEN]+password[MSG_STRING_LEN]
    MSG_TODO_USER_LOGOUT,//登出//none
    MSG_TODO_USER_CHANGED_PASSWORD,//修改密码//newpassword[MSG_STRING_LEN]
    MSG_TODO_USER_LIST,//列表//none
    MSG_TODO_USER_DELTE,//删除用户//userid[32]
    MSG_TODO_USER_ADD,//添加用户//username[MSG_STRING_LEN] password[MSG_STRING_LEN] role[1]
    MSG_TODO_USER_MODIFY,//修改用户//id[4] username[MSG_STRING_LEN] password[MSG_STRING_LEN] role[1]
    MSG_TODO_MAP_CREATE_START,//创建地图开始
    MSG_TODO_MAP_CREATE_ADD_STATION,//添加站点 station[id[4]+x[4]+y[4]+name[MSG_STRING_LEN]+rfid[4]+r[2]+g[2]+b[2]]{个} //如果超出1024长度，可以分成多条
    MSG_TODO_MAP_CREATE_ADD_LINE, //添加直线 line[id[4] + startstation[4] + endstation[MSG_STRING_LEN] + length[4] + draw[1] + r[2] + g[2] + b[2]]{个 }//如果超出1024长度，可以分成多条
    MSG_TODO_MAP_CREATE_FINISH,//创建地图完成
    MSG_TODO_MAP_LIST_STATION,//请求所有站点//none
    MSG_TODO_MAP_LIST_LINE,//请求所有直线//none
    //MSG_TODO_HAND_REQUEST,//请求控制权//none
    //MSG_TODO_HAND_RELEASE,//释放控制权//none
    //MSG_TODO_HAND_FORWARD,//前进//none
    //MSG_TODO_HAND_BACKWARD,//后退//none
    //MSG_TODO_HAND_TURN_LEFT,//左转//none
    //MSG_TODO_HAND_TURN_RIGHT,//右转//none
    MSG_TODO_AGV_MANAGE_LIST,//车辆列表//none
    MSG_TODO_AGV_MANAGE_ADD,//增加//name[MSG_STRING_LEN]+ip[MSG_STRING_LEN]+port[4]
    MSG_TODO_AGV_MANAGE_DELETE,//删除//id[4]
    MSG_TODO_AGV_MANAGE_MODIFY,//修改//id[4]+name[MSG_STRING_LEN]+ip[MSG_STRING_LEN]
    MSG_TODO_TASK_CREATE,//添加任务
    MSG_TODO_TASK_QUERY_STATUS,//查询任务状态//taskid[4]
    MSG_TODO_TASK_CANCEL,//取消任务//taskid[4]
    MSG_TODO_TASK_LIST_UNDO,//未完成的列表//none
    MSG_TODO_TASK_LIST_DOING,//正在执行的任务列表//none
    MSG_TODO_TASK_LIST_DONE_TODAY,//今天完成的任务//none
    MSG_TODO_TASK_LIST_DURING,//历史完成的任务//时间格式格式yyyy-MM-dd hh-mm-ss；from_time[24] to_time[24]
    MSG_TODO_LOG_LIST_DURING,//查询历史日志//时间格式格式yyyy-MM-dd hh-mm-ss；from_time[24] to_time[24]
    MSG_TODO_SUB_AGV_POSITION,//订阅车辆位置信息
    MSG_TODO_CANCEL_SUB_AGV_POSITION,//取消车辆位置信息订阅
    MSG_TODO_SUB_AGV_STATSU,//订阅车辆状态信息
    MSG_TODO_CANCEL_SUB_AGV_STATSU,//取消车辆状态信息订阅
    MSG_TODO_SUB_LOG,//订阅日志
    MSG_TODO_CANCEL_SUB_LOG,//取消日志订阅
    MSG_TODO_SUB_TASK,//任务订阅
    MSG_TODO_CANCEL_SUB_TASK,//取消任务订阅
	MSG_TODO_TRAFFIC_CONTROL_STATION,//交通管制,一个站点//stationId[int32]
	MSG_TODO_TRAFFIC_CONTROL_LINE,//交通管制，一条线路//lineId[int32]
	MSG_TODO_TRAFFIC_RELEASE_STATION,//交通管制 释放,一个站点//stationId[int32]
	MSG_TODO_TRAFFIC_RELEASE_LINE,//交通管制 释放，一条线路//lineId[int32]

    //publish request and response
    MSG_TODO_PUB_AGV_POSITION,//发布的agv位置信息，该信息的queuebumber = 0
    MSG_TODO_PUB_AGV_STATUS,//发布的agv状态信息，该信息的queuebumber = 0
    MSG_TODO_PUB_LOG,//发布的日志信息，该信息的queuebumber = 0
    MSG_TODO_PUB_TASK,//发布的任务信息，该信息的queuebumber = 0

    //notify
    MSG_TODO_NOTIFY_ALL_MAP_UPDATE,//通知消息 -- 地图更新
    MSG_TODO_NOTIFY_ALL_ERROR,
}MSG_TODO;

typedef struct _NOTIFY_ERROR
{
    bool needConfirm;
    int32_t code;
    char msg[MSG_RESPONSE_BODY_MAX_SIZE-4-sizeof(bool)];
}NOTIFY_ERROR;

//定义消息头的 todo//---------------------------------------------------------------------------------------------------------------------------------
////////////////////////////////////////以下是特殊情况的返回结构体

//用户信息结构体[登录成功时，返回一个该用户的userinfo.用户列表返回多个用户userinfo]

enum{
    USER_ROLE_VISITOR = 0,//游客，只有登录的权限
    USER_ROLE_OPERATOR,//普通操作人员
    USER_ROLE_ADMIN,//管理员
    USER_ROLE_SUPER_ADMIN,//超级管理员
    USER_ROLE_DEVELOP,//开发人员
};


typedef struct _USER_INFO
{
    uint32_t id;//id号
    uint32_t role;//角色
    char username[MSG_STRING_LEN];//用户名
    char password[MSG_STRING_LEN];//密码
    uint8_t status;//登录状态
}USER_INFO;

//AGV基本信息
typedef struct _AGV_BASE_INFO
{
    uint32_t id;
    char name[MSG_STRING_LEN];
    char ip[MSG_STRING_LEN];
    uint32_t port;
}AGV_BASE_INFO;

//AGV位置信息
typedef struct _AGV_POSITION_INFO
{
    uint32_t id;
    uint32_t x;
    uint32_t y;
    int32_t rotation;
}AGV_POSITION_INFO;


typedef struct _STATION_INFO
{
    int32_t id;
    int32_t x;
    int32_t y;
    int32_t floorId;
    int32_t occuagv;
    char name[MSG_STRING_LEN];
}STATION_INFO;

typedef struct _AGV_LINE
{
    int32_t id;
    int32_t startStation;
    int32_t endStation;
    int32_t length;
}AGV_LINE;

typedef struct _TASK_INFO
{
    int32_t id;
    char produceTime[MSG_TIME_STRING_LEN];
    char doTime[MSG_TIME_STRING_LEN];
    char doneTime[MSG_TIME_STRING_LEN];
    int32_t excuteAgv;
    int32_t status;
}TASK_INFO;

//用户日志，显示给用户的日志。
typedef struct _USER_Log {
    char time[MSG_TIME_STRING_LEN];//yyyy-MM-dd hh:mm:ss.fff
    char msg[MSG_LOG_MAX_LENGTH];//日志内容
}USER_LOG;

#pragma pack ()
