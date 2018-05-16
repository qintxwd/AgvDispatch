#include "common.h"
#include <iomanip>

ThreadPool g_threadPool(30);
CppSQLite3DB g_db;

const int GLOBAL_AGV_PROJECT = AGV_PROJECT_QINGDAO;

std::string getTimeStrNow()
{
    using std::chrono::system_clock;

    //获取当前时间
    auto time_now = std::chrono::system_clock::now();
    //计算当前时间的 毫秒数 %1000;
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch()).count()%1000;

    std::time_t tt = std::chrono::system_clock::to_time_t(time_now);

    std::stringstream ss;


    //gcc 4.9及之前版本不支持 std::put_time//需要gcc 5.0+
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms;

    return ss.str();
}

std::string getTimeStrToday()
{
    using std::chrono::system_clock;

    //获取当前时间
    auto time_now = std::chrono::system_clock::now();

    std::time_t tt = std::chrono::system_clock::to_time_t(time_now);

    std::stringstream ss;

    //gcc 4.9及之前版本不支持 std::put_time//需要gcc 5.0+
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d 00:00:00") << "." << 0;

    return ss.str();
}
std::string getTimeStrTomorrow()
{
    using std::chrono::system_clock;

    //获取当前时间
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<int,std::ratio<60*60*24> > one_day(1);
    system_clock::time_point tomorrow = time_now + one_day;

    std::time_t tt = std::chrono::system_clock::to_time_t(tomorrow);

    std::stringstream ss;

    //gcc 4.9及之前版本不支持 std::put_time//需要gcc 5.0+
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d 00:00:00") << "." << 0;

    return ss.str();
}

std::string toHexString(char *data,int len)
{
    std::stringstream out;
    out<<std::hex<<std::setfill('0');
    for(int i=0;i<len;++i){
        out<<std::setw(2)<<(0xff &(data[i]))<<std::setw(1)<<" ";
    }
    return out.str();
}

std::string intToString(int i)
{
    std::stringstream out;
    out<<i;
    return out.str();
}

int stringToInt(std::string str)
{
    std::stringstream out;
    out<<str;
    int i;
    out>>i;
    return i;
}

bool stringToBool(std::string str)
{
    if(str.length()<=0)return false;
    if(str == "0"||str == "false")return false;
    return true;
}

std::vector<std::string> splite(std::string src,std::string sp)
{
    std::vector<std::string> result;
    if(sp.length()==0){
        result.push_back(src);
        return result;
    }

    size_t pos;
    while(true){
        pos = src.find_first_not_of(sp);
        if(pos == std::string::npos){
            break;
        }
        result.push_back(src.substr(0,pos));
        src = src.substr(pos+sp.length());
    }
    result.push_back(src);
    return result;
}

#ifndef _WIN32

void memcpy_s(void *__restrict __dest, size_t __m,const void *__restrict __src, size_t __n)
{
    memcpy(__dest,__src,__m<__n?__m:__n);
}


const  int  G_AGV_TYPE = AGV_TYPE_JACKING;

#endif


int  GLOBAL_AGV_TYPE = AGV_TYPE_JACKING;