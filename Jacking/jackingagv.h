#ifndef JACKINGAGV_H
#define JACKINGAGV_H

#include "agv.h"
class JackingAgv;
using JackingAgvPtr = std::shared_ptr<JackingAgv>;

class JackingAgv : public Agv
{
public:
    JackingAgv(int id,std::string name,std::string ip,int port);

    enum { Type = Agv::Type+1 };

    int type(){return Type;}


    //车辆旋转//返回发送是否成功
    bool turn(bool left,int angle);
    //等待旋转结束，如果接收失败，返回false。返回接受的结果
    bool waitTurnEnd(int waitMs);

    //车辆前进//返回发送是否成功
    bool forward(int mm);
    bool waitForwardEnd(int waitMs);

    //车辆后退
    bool back(int mm);
    bool waitBackEnd(int waitMs);

    void onRead(const char *data,int len);
private:
    static const int maxResendTime = 10;

    bool resend(const char *data,int len);

    bool turnRecv;
    bool turnResult;

    bool forwardRecv;
    bool forwardResult;

    bool backRecv;
    bool backResult;
};

#endif // JACKINGAGV_H
