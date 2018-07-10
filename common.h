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
#include <limits.h>
#include "utils/Log/spdlog/spdlog.h"
#include "utils/Log/spdlog/fmt/ostr.h"
#include "utils/threadpool.h"
#include "sqlite3/CppSQLite3.h"

#define  CAN_CHANGE_DIRECTION  false

#define  DISTANCE_INFINITY INT_MAX

#define DB_File ("agv.db")

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
int stringToInt(std::string str);
bool stringToBool(std::string str);

std::vector<std::string> split(std::string src,std::string sp = " ");
std::vector<std::string> splitMultiJson(std::string multiJson);
bool IsValidIPAddress(const char * str);
int HexStringToInt(std::string str);

extern ThreadPool g_threadPool;
extern CppSQLite3DB g_db;

#ifndef _WIN32
void memcpy_s(void *__restrict __dest, size_t __m,const void *__restrict __src, size_t __n);
#include<unistd.h>
#define Sleep(value) usleep(value * 1000);
#else
#define sleep(value) Sleep(value * 1000);
#endif

//考虑到有些项目可能是多种车一起调度，那么根据项目来区分不同的taskmaker
enum{
    AGV_PROJECT_QUNCHUANG = 0,
    AGV_PROJECT_DONGYAO,
    AGV_PROJECT_QINGDAO,
    //...
};

#define QUNCHUANG_PROJECT

extern const int GLOBAL_AGV_PROJECT;


#endif // COMMON_H
