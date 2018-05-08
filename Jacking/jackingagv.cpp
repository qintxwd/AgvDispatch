#include "jackingagv.h"
#include "../common.h"
JackingAgv::JackingAgv(int id, std::string name, std::string ip, int port):
    Agv(id,name,ip,port)
{

}

bool JackingAgv::resend(const char *data,int len){
    if(!data||len<=0)return false;
    bool sendResult = send(data,len);

    int resendTimes = 0;
    while(!sendResult && ++resendTimes< maxResendTime){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(500));
        sendResult = send(data,len);
    }

    return sendResult;
}

bool JackingAgv::turn(bool left, int angle)
{
    std::stringstream ss;
    ss<<"turn ";
    if(left){
        ss<<"1 ";
    }else{
        ss<<" ";
    }
    ss<<angle;
    turnRecv = false;
    turnResult = false;
    return resend(ss.str().c_str(),ss.str().length());
}

bool JackingAgv::waitTurnEnd(int waitMs)
{
    while(!turnRecv && --waitMs>0){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(1));
    }

    if(!turnRecv)return false;
    return turnResult;
}

bool JackingAgv::forward(int mm)
{
    std::stringstream ss;
    ss<<"forward ";
    ss<<mm;
    forwardRecv = false;
    forwardResult = false;
    return resend(ss.str().c_str(),ss.str().length());
}

bool JackingAgv::waitForwardEnd(int waitMs)
{
    while(!forwardRecv && --waitMs>0){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(1));
    }

    if(!forwardRecv)return false;
    return forwardResult;
}



bool JackingAgv::back(int mm)
{
    std::stringstream ss;
    ss<<"backward ";
    ss<<mm;
    backRecv = false;
    backResult = false;
    return resend(ss.str().c_str(),ss.str().length());
}

bool JackingAgv::waitBackEnd(int waitMs)
{
    while(!backRecv && --waitMs>0){
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(1));
    }

    if(!backRecv)return false;
    return backResult;
}

void JackingAgv::onRead(const char *data,int len)
{
    if(data == NULL || len <= 0)return ;
    std::string msg(data,len);
    if(msg.find("turn ")== 0){
        turnRecv = true;
        if(msg.find("turn 1")==0){
            turnResult = true;
        }else{
            turnResult = false;
        }
    }

    if(msg.find("forward ")== 0){
        forwardRecv = true;
        if(msg.find("forward 1")==0){
            forwardResult = true;
        }else{
            forwardResult = false;
        }
    }

    if(msg.find("backward ")== 0){
        backRecv = true;
        if(msg.find("backward 1")==0){
            backResult = true;
        }else{
            backResult = false;
        }
    }
}
