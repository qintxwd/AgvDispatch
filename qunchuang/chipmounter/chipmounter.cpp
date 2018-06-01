#include "chipmounter.h"

chipmounter::chipmounter(int _id, std::string _name, std::string _ip, int _port) :
    Device(_id,_name,_ip, _port)
{
     loading_finished = false;
     unloading_finished = false;
}

chipmounter::~chipmounter()
{
    if (tcpClient)
        delete tcpClient;
}


void chipmounter::onConnect()
{
    combined_logger->info("偏贴机, connect ok!!! ");
    connected = true;

    //NotifyAGVArrived(0xFFFF, 0xABCD);
}

void chipmounter::onDisconnect()
{
    combined_logger->info("偏贴机, disconnect !!! ");
    connected = false;

}

bool chipmounter::startLoading(int16_t id)
{
    std::unique_lock <std::mutex> lock(rolling_mutex);

    loading_finished = false;
    NotifyAGVArrived(id,AGV_LOADING_ARRVIED);
    //wait for PLC告知转动信息
    rolling_status.wait(lock);


}


bool chipmounter::startUnLoading(int16_t id)
{
     unloading_finished = false;
}

void chipmounter::onRead(const char *data,int len)
{
    int16_t buffer[2];

    char str_received[9];

    //combined_logger->info("偏贴机, data len: %d", len);
    std::cout<<"偏贴机, data len: "<< len <<std::endl;


    if(len == 4)
    {
        bytesToHexstring(data, 4, str_received, 8);
        str_received[9]='\0';

        buffer[0] = (int16_t)data[0];
        buffer[1] = (int16_t)data[2];

        if(buffer[1] == CHIPMOUNTER_SATRT_ROLLING)
        {
            OnDeviceStartRolling();
        }
        else if(buffer[1] == LOADING_FINISHED)
        {
            OnloadFinished();
        }
        else if(buffer[1] == UNLOADING_FINISHED)
        {
            OnUnloadFinished();
        }
        else
        {
            combined_logger->error("偏贴机, data error: "  + string(str_received));
        }
    }
    else
    {
        combined_logger->error("偏贴机, data length error: "  + string(str_received));
    }
}



/* AGV已到达
 *id: 工位 ID,  点位
 *task:  (AGV_LOADING_ARRVIED, 上料到达)  (AGV_UNLOADING_ARRVIED, 下料到达)
 */
void chipmounter::NotifyAGVArrived(int16_t id, int16_t task)
{
    int16_t data[2];
    data[0] = id;
    data[1] = task;

    if(tcpClient != nullptr)
    {
        this->send((char*)data, sizeof(data));
    }
    else
        combined_logger->error("偏贴机, tcpClient is null ");

}


/*
 * PLC告知转动信息
 */
void chipmounter::OnDeviceStartRolling()
{
    combined_logger->info("偏贴机, 告知转动");
    rolling_status.notify_all();
}


/*
 * PLC告知上料完成
 */
void chipmounter::OnloadFinished()
{
    combined_logger->info("偏贴机, 告知上料完成");
    loading_finished = true;
}


/*
 * PLC告知AGVC下料完成
 */
void chipmounter::OnUnloadFinished()
{
    combined_logger->info("偏贴机, 告知下料完成");
    unloading_finished = true;
}

