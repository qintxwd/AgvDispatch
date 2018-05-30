#ifndef CHIPMOUNTER_H
#define CHIPMOUNTER_H
#include "device/device.h"

//AGV已到达(上料到达), (点位0x0000~0xFFFF)+(消息类别 0x0000)
#define AGV_LOADING_ARRVIED        0x0000
//PLC告知AGVC转动信息 (点位0x0000~0xFFFF)+(消息类别 0x0010)
#define CHIPMOUNTER_SATRT_ROLLING  0x0010
//PLC告知AGVC上料完成, (点位0x0000~0xFFFF)+(消息类别 0x0020)
#define LOADING_FINISHED           0x0020
//AGV已到达(下料到达), (点位0x0000~0xFFFF)+(消息类别 0x0001)
#define AGV_UNLOADING_ARRVIED      0x0001
//PLC告知AGVC上料完成, (点位0x0000~0xFFFF)+(消息类别 0x0021)
#define UNLOADING_FINISHED         0x0021

//偏贴机client class
class chipmounter : public Device
{
public:

    chipmounter(int id,std::string name,std::string ip,int port);
    virtual ~chipmounter();

    bool startLoading(int16_t id, int16_t task);
    bool startUnLoading(int16_t id, int16_t task);

    /* AGV已到达
     *id: 工位 ID,  点位
     *task:  (AGV_LOADING_ARRVIED, 上料到达)  (AGV_UNLOADING_ARRVIED, 下料到达)
     */
    void NotifyAGVArrived(int16_t id, int16_t task);


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


    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();



};



#endif // CHIPMOUNTER_H
