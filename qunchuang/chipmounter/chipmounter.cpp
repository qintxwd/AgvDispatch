#include "chipmounter.h"
//#include "netinet/in.h"
//#include <boost/lexical_cast.hpp>

chipmounter::chipmounter(int _id, std::string _name, std::string _ip, int _port) :
    Device(_id,_name,_ip, _port)
{
     loading_finished = false;
     unloading_finished = false;
}

chipmounter::~chipmounter()
{
    if (tcpClient!=nullptr)
    {
        delete tcpClient;
        tcpClient=nullptr;
    }
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

bool chipmounter::startLoading(int16_t id, int agv_arrivied_info)
{   
    combined_logger->info("chipmounter, startLoading...");
    std::unique_lock <std::mutex> lock(rolling_mutex);
    combined_logger->info("chipmounter, NotifyAGVArrived, agv_arrivied_info: {0}", agv_arrivied_info);

    loading_finished = false;
    NotifyAGVArrived(id,agv_arrivied_info);
    //wait for PLC告知转动信息
    combined_logger->info("chipmounter, wait for PLC告知转动信息...");

    rolling_status.wait(lock);
    combined_logger->info("chipmounter, PLC告知转动信息..., startLoading end");
    return true;
}


bool chipmounter::startUnLoading(int16_t id)
{
     unloading_finished = false;
     NotifyAGVArrived(id,AGV_UNLOADING_ARRVIED);
     return true;
}

void chipmounter::onRead(const char *data,int len)
{
    string buffer[2];

    char str_received[9];
    string temp;

    //combined_logger->info("偏贴机, data len: %d", len);
    std::cout<<"偏贴机, data len: "<< len <<std::endl;


    if(len == 4)
    {
        bytesToHexstring(data, 4, str_received, 8);
        str_received[9]='\0';

        temp = string(str_received);

        combined_logger->info("偏贴机, data temp: "  + temp);


        buffer[1] = temp.substr(2,2) + temp.substr(0,2);
        buffer[0] = temp.substr(6,2) + temp.substr(4,2);;

        combined_logger->info("偏贴机, buffer[0]: "  + buffer[0]);
        combined_logger->info("偏贴机, buffer[1]: "  + buffer[1]);


        if(buffer[1] == CHIPMOUNTER_SATRT_ROLLING_STRING)
        {
            OnDeviceStartRolling();
        }
        else if(buffer[1] == LOADING_FINISHED_STRING)
        {
            OnloadFinished();
        }
        else if(buffer[1] == UNLOADING_FINISHED_STRING)
        {
            OnUnloadFinished();
        }
        else if(buffer[1] == UNLOADING_INFO)
        {
            combined_logger->info("偏贴机, PLC告知DCS取空卡塞");
            OnUnloading(buffer[0]);

            if("2510" == buffer[0])
                combined_logger->info("偏贴机, 2F Polar-06");
            else if("2511" == buffer[0]){
                combined_logger->info("偏贴机, 2F Polar-07");
            }
        }
        else if(buffer[1] == LOADING_INFO)
        {
            combined_logger->info("偏贴机, PLC告知DCS上料");
            OnLoading(buffer[0]);

            if("2510" == buffer[0])
                combined_logger->info("偏贴机, 2F Polar-06");
            else if("2511" == buffer[0]){
                combined_logger->info("偏贴机, 2F Polar-07");
            }
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
    data[1] = id;
    data[0] = task;

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

