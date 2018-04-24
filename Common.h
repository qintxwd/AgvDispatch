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
#include "utils/Log/easylogging.h"
#include "utils/threadpool.h"
#include "sqlite3/CppSQLite3.h"

#define  CAN_CHANGE_DIRECTION  false

#define  DISTANCE_INFINITY INT_MAX

#define TRAFFIC_OCCUR_AGV_ID			(999999)

#define DB_File ("agv.db")

typedef std::chrono::duration<int,std::milli> duration_millisecond;
typedef std::chrono::duration<int> duration_second;
typedef std::chrono::duration<int,std::ratio<60*60>> duration_hour;
typedef std::unique_lock<std::mutex>  UNIQUE_LCK;
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

extern ThreadPool g_threadPool;
extern CppSQLite3DB g_db;

#ifndef _WIN32

void memcpy_s(void *__restrict __dest, size_t __m,const void *__restrict __src, size_t __n);

#endif

#endif // COMMON_H
