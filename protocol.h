#pragma once

//修改协议
/*
 * 修改
 * 时间 2018-5-8
 * 秦英豪
 * 内容: 修改现有协议，改为 json 协议。 json的可扩展性好些
 * 新版协议
 * |HEAD        | LENGTH        |json data with length      |
 * |1Byte       | 4Byte         | ...                       |
 * |固定为0x88   | 标记json的长度  | json数据                     |
 * json:根节点 必须包含如下几个信息
 * type: 0.request  1.response  2.publish  3.notify  [request 是client到server的请求，其他为 server到client的]
 * todo:要做的事
 * queuenumber:请求序号
 * */


#include <cstdint>
#include <string>
#include <json/json.h>

//define msg from client to server and msg from server to client
// make each msg length <= 1024

#define GLOBAL_SERVER_PORT   9999

#define MSG_MSG_HEAD		(0x88)

//小于这个值，直接在内存中读取，大于这个值，写入临时文件，读取临时文件
#define MSG_READ_BUFFER_LENGTH  (1024)  //读取缓存区的 大小
#define MSG_JSON_PREFIX_LENGTH  (5)     //一个0x88 还有4个字节的一个int，标记json_length
#define MSG_JSON_MEMORY_LENGTH  (1000)  //json长度超出这个，就由文件缓存

#define SQL_MAX_LENGTH                  (2048)

//消息类型
typedef enum Msg_Type {
    MSG_TYPE_REQUEST = 0,//客户端请求
    MSG_TYPE_RESPONSE,//服务器应答
    MSG_TYPE_PUBLISH,//服务器发布
    MSG_TYPE_NOTIFY//服务器通知
}MSG_TYPE;

//消息做的事情
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
    MSG_TODO_MAP_SET_MAP,//创建地图开始
    MSG_TODO_MAP_GET_MAP,//请求所有站点//none
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

//应答 是否成功
enum
{
    RETURN_MSG_RESULT_SUCCESS = 0,//全局的成功
    RETURN_MSG_RESULT_FAIL,//全局的错误
};

//应答 错误代码
enum {
    RETURN_MSG_ERROR_NO_ERROR = 0,
    RETURN_MSG_ERROR_CODE_UNKNOW,//未知错误
    RETURN_MSG_ERROR_CODE_PARAMS,//数据长度有问题
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

//用户角色
enum{
    USER_ROLE_VISITOR = 0,//游客，只有登录的权限
    USER_ROLE_OPERATOR,//普通操作人员
    USER_ROLE_ADMIN,//管理员
    USER_ROLE_SUPER_ADMIN,//超级管理员
    USER_ROLE_DEVELOP,//开发人员
};

//用户日志，显示给用户的日志。
typedef struct _USER_Log {
    std::string time;//yyyy-MM-dd hh:mm:ss.fff
    std::string msg;//日志内容
}USER_LOG;

