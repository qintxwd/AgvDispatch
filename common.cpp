#include "common.h"
#include <iomanip>

ThreadPool g_threadPool(30);
CppSQLite3DB g_db;

//const int GLOBAL_AGV_PROJECT = AGV_PROJECT_QINGDAO;
const int GLOBAL_AGV_PROJECT = AGV_PROJECT_QUNCHUANG;

std::shared_ptr<spdlog::logger> combined_logger;

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

int HexStringToInt(std::string hexStr)
{
    int i;
    std::string ss = "0x" + hexStr;
    sscanf(ss.c_str(),"%X",&i);
    return i;
}

bool stringToBool(std::string str)
{
    if(str.length()<=0)return false;
    if(str == "0"||str == "false")return false;
    return true;
}

std::vector<std::string> split(std::string src,std::string sp)
{
    std::vector<std::string> result;
	if (src.length() == 0)return result;
    if(sp.length()==0){
        result.push_back(src);
        return result;
    }

    size_t pos;
    while(true){
        pos = src.find_first_of(sp);
        if(pos == std::string::npos){
            break;
        }
		if(pos!=0)
			result.push_back(src.substr(0,pos));
        src = src.substr(pos+sp.length());
    }
	if(src.length()>0)
		result.push_back(src);
    return result;
}

/*
 * multiJson like this:
{"topic": "/waypoint_user_sub", "msg": {"data": "charge_status:0"}, "op": "publish"}
{"topic": "/waypoint_user_sub", "msg": {"data": "emergency_status:1,1,1"}, "op": "publish"}
*/

std::vector<std::string> splitMultiJson(std::string multiJson)
{
    const char * data = multiJson.c_str();
    int left = 0;
    int right = 0;

    //combined_logger->info("splitMultiJson, data: "+ multiJson);


    std::vector<std::string> result;
    if (multiJson.length() == 0)
        return result;

    if(*data != '{' /*|| *(data + (multiJson.size()-1))!= '}'*/)
        return result;

    size_t pos=0;
    while(*(data+pos) != '\0'){
        if(*(data+pos) == '{')
        {
            left++;
        }

        if(*(data+pos) == '}')
        {
            right++;
        }

        pos++;

        if(left == right && left != 0)
        {
            std::string json = multiJson.substr(0,pos);
            result.push_back(json);

            multiJson = multiJson.substr(pos, multiJson.length());
            data = multiJson.c_str();
            pos = 0;
            left = 0;
            right = 0;
            continue;
        }
    }

    if(multiJson.size() > 0)
    {
        //combined_logger->error("splitMultiJson, not json: "+ multiJson);

        result.push_back(multiJson);
    }

    return result;
}

bool IsValidIPAddress(const char * str){
    //先判断形式是否合法，

    //检查是否只包含点和数字
    for(int i = 0; str[i] != '\0'; i++){
        if(!isdigit(str[i]) && str[i] != '.')
            return false;
    }

    //检查是否形如X.X.X.X
    int count = 0;
    for(int i = 0; str[i+1] != '\0'; i++){
        if(isdigit(str[i]) && str[i+1] == '.' )
            count++;
    }
    if(count != 3)
        return false;

    //检查区间是否合法
    int temp = 0;
    int j = 0;
    for(int i = 0; str[i] != '\0'; i++){
        if(str[i] != '.'){
            temp = (temp * 10  + int(str[i] - '0'));
            j++;
        }
        else{
            if(temp <= 255){
                temp = 0;
                j = 0;
            }
            else
                return false;
        }
    }

    //最后一个也要判断
    if(temp > 255)
        return false;

    //通过所有测试，返回正确
    return true;
}


#ifndef _WIN32

void memcpy_s(void *__restrict __dest, size_t __m,const void *__restrict __src, size_t __n)
{
    memcpy(__dest,__src,__m<__n?__m:__n);
}

#endif

