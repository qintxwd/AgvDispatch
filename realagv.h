#ifndef REALAGV_H
#define REALAGV_H

#include "agv.h"

class TcpClient;

//继承 realAgv 需要 自己实现的虚函数包括:
/////////////////必须实现的
//1.type()              //用于区分不同类型的agv
//6.excutePath()        //让agv执行路径
//8.goStation()         //让agv去某个站点，一般是excutePath中调用该函数
//9.stop()              //用于中控 手动停车或取消任务时调用
//10.callMapChange()    //用于切换地图，例如进出电梯、在电梯中切换楼层
//7.arrive()            //通知agv当前位置信息，用于显示agv位置
//2.onRead()            //处理agv发送上来的消息数据
//5.cancelTask()        //通知agv任务取消
///////////////不必须实现的
//0.init()          //可以不实现//初始化、这里已经初始化了到agv的socket连接
//3.onConnect()     //可以不实现
//4.onDisconnect()  //可以不实现
//13.onError()      //可以不实现
//14.onWarning()    //可以不实现
///////////////必须在必要的时候调用
//11.onArriveStation()      //用于到达一个站点，释放前面经过的线路
//12.onLeaveStation()       //用于释放前面刚刚经过的站点

class RealAgv : public Agv
{
public:
    RealAgv(int id,std::string name,std::string ip,int port);
    virtual ~RealAgv();
    virtual void init();
    //回调
    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();

    virtual void reconnect();
    virtual bool send(const char *data, int len);

protected:
     TcpClient *tcpClient;
};

#endif // REALAGV_H
