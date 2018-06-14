#ifndef CHIPMOUNTER_H
#define CHIPMOUNTER_H
#include "device/device.h"

//AGV已到达(上料到达), (点位0x0000~0xFFFF)+(消息类别 0x0000)
#define AGV_LOADING_ARRVIED        0x0000
#define AGV_LOADING_ARRVIED_STRING        "0000"
//PLC告知AGVC转动信息 (点位0x0000~0xFFFF)+(消息类别 0x0010)
#define CHIPMOUNTER_SATRT_ROLLING   0x0010
#define CHIPMOUNTER_SATRT_ROLLING_STRING  "0010"
//PLC告知AGVC上料完成, (点位0x0000~0xFFFF)+(消息类别 0x0020)
#define LOADING_FINISHED           0x0020
#define LOADING_FINISHED_STRING           "0020"
//AGV已到达(下料到达), (点位0x0000~0xFFFF)+(消息类别 0x0001)
#define AGV_UNLOADING_ARRVIED     0x0001
#define AGV_UNLOADING_ARRVIED_STRING      "0001"
//PLC告知AGVC下料完成, (点位0x0000~0xFFFF)+(消息类别 0x0021)
#define UNLOADING_FINISHED         0x0021
#define UNLOADING_FINISHED_STRING         "0021"

//PLC告知DCS取空卡塞   (点位0x0000~0xFFFF, 0x2510 2F Polar-06)+(消息类别 0x0030)
#define UNLOADING_INFO         "0030"

//PLC告知DCS上料   (点位0x0000~0xFFFF, 0x2511 2F Polar-07)+(消息类别 0x0031)
#define LOADING_INFO         "0040"


struct chipinfo
{
    string point;
    string action;
};

//偏贴机client class
class chipmounter : public Device
{
public:

    chipmounter(int id,std::string name,std::string ip,int port);
    virtual ~chipmounter();

    bool startLoading(int16_t id);
    bool startUnLoading(int16_t id);
    bool isLoadingFinished()
    {
        return loading_finished;
    }

    bool isUnLoadingFinished()
    {
        return unloading_finished;
    }

    /* AGV已到达
     *id: 工位 ID,  点位
     *task:  (AGV_LOADING_ARRVIED, 上料到达)  (AGV_UNLOADING_ARRVIED, 下料到达)
     */
    void NotifyAGVArrived(int16_t id, int16_t task);

    bool getAction(chipinfo* c)
    {
        chipinfo info;

        if(chip_info.empty())
            return false;
        else
        {
            info = chip_info.at(0);
            //chip_info.erase(chip_info.begin());//删除第一个元素
            c->action = info.action;
            c->point = info.point;
            return true;
        }
    }

    void deleteHeadAction()
    {
        if(!chip_info.empty())
            chip_info.erase(chip_info.begin());//删除第一个元素
    }

private:


    /*
     * PLC告知转动信息
     */
    void OnDeviceStartRolling();


    /*
     * PLC告知上料完成
     */
    void OnloadFinished();


    /*
     * PLC告知AGVC下料完成
     */
    void OnUnloadFinished();


    /*
     * PLC告知DCS取空卡塞
     */
    void OnUnloading(string point)
    {
        chipinfo info;
        info.point = point;
        info.action = "unloading"; //取空卡塞

        chip_info.push_back(info);
    }

    /*
     * PLC告知DCS上料
     */
    void OnLoading(string point)
    {
        chipinfo info;
        info.point = point;
        info.action = "loading"; //上料

        chip_info.push_back(info);
    }




    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();


    std::mutex rolling_mutex;
    std::condition_variable rolling_status; // rolling_status条件变量.

    bool loading_finished;
    bool unloading_finished;

    std::vector<chipinfo> chip_info;

};



#endif // CHIPMOUNTER_H
