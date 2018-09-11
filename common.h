#ifndef COMMON_H
#define COMMON_H
#include <vector>
#include <list>
#include <chrono>
#include <ratio>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <algorithm>
#include <memory>
#include <limits>
#include <limits.h>
#include "utils/Log/spdlog/spdlog.h"
#include "utils/Log/spdlog/fmt/ostr.h"
#include "utils/threadpool.h"
#include "sqlite3/CppSQLite3.h"

#ifdef WIN32
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif

#define  CAN_CHANGE_DIRECTION  false

#define  DISTANCE_INFINITY INT_MAX
#define  DISTANCE_INFINITY_DOUBLE   ((std::numeric_limits<double>::max)())
#define DB_File ("agv.db")

#define AGV_TYPE_VIRTUAL_ROS_AGV        -1
#define AGV_TYPE_ANTING_FORKLIFT         1

typedef std::chrono::duration<int,std::milli> duration_millisecond;
typedef std::chrono::duration<int> duration_second;
typedef std::chrono::duration<int,std::ratio<60*60>> duration_hour;
typedef std::unique_lock<std::mutex>  UNIQUE_LCK;

extern std::shared_ptr<spdlog::logger> combined_logger;

enum {
    AGV_LINE_COLOR_WHITE = 0,  //未算出路径最小值
    AGV_LINE_COLOR_GRAY,       //已经计算出一定的值，在Q队列中，但是尚未计算出最小值
    AGV_LINE_COLOR_BLACK,      //已算出路径最小值
};

std::string getTimeStrNow();
std::string getTimeStrToday();
std::string getTimeStrTomorrow();

std::string toHexString(char *data, int len);
std::string intToString(int i);
std::string longToString(long l);
int stringToInt(std::string str);
bool stringToBool(std::string str);

std::vector<std::string> split(std::string src,std::string sp = " ");
std::vector<std::string> splitMultiJson(std::string multiJson);
bool IsValidIPAddress(const char * str);
int HexStringToInt(std::string str);

//东药名匠公用
double func_dis(int x1, int y1, int x2, int y2);
int func_angle(int a1, int a2);
std::string transToFullMsg(const std::string &data);

class Pose4D
{
public:
    Pose4D(){}
    Pose4D(double x, double y, double theta, int floor)
    {
        m_x = x;
        m_y = y;
        m_theta = theta;
        m_floor = floor;
    }
    double m_x;
    double m_y;
    double m_theta;
    int m_floor;

};

class DyMsg
{
public:
    std::string msg;
    int waitTime;
};

class TimeUsed{
public:
    void start()
    {
        s = std::chrono::steady_clock::now();
    }
    void end()
    {
        e = std::chrono::steady_clock::now();
    }
    double getUsed()
    {
        std::chrono::duration<double> time_used = std::chrono::duration_cast<std::chrono::duration<double>>(e - s);
        return time_used.count();
    }
private:
    std::chrono::steady_clock::time_point s;
    std::chrono::steady_clock::time_point e;
};

extern ThreadPool g_threadPool;
extern CppSQLite3DB g_db;

#ifndef _WIN32
void memcpy_s(void *__restrict __dest, size_t __m,const void *__restrict __src, size_t __n);
#include<unistd.h>
#define Sleep(value) usleep(value * 1000)
#else
#define sleep(value) Sleep(value * 1000)
#define usleep(value) Sleep(value/1000)
#endif

//考虑到有些项目可能是多种车一起调度，那么根据项目来区分不同的taskmaker
enum{
    AGV_PROJECT_QUNCHUANG = 0,
    AGV_PROJECT_DONGYAO,
    AGV_PROJECT_QINGDAO,
    AGV_PROJECT_ANTING
    //...
};

extern const int GLOBAL_AGV_PROJECT;
extern std::atomic<bool> g_quit;

#endif // COMMON_H
