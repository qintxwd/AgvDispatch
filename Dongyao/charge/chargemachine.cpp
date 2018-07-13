#include "chargemachine.h"


chargemachine::chargemachine(int _id, std::string _name, std::string _ip, int _port) :
    Device(_id,_name,_ip, _port)
{
    charging_finished = false;
    timeout = 500;
}

chargemachine::~chargemachine()
{
    //    if (tcpClient)
    //        delete tcpClient;
}

void chargemachine::onConnect()
{
    combined_logger->info("充电机, connect ok!!! ");
    connected = true;
}

void chargemachine::onDisconnect()
{
    combined_logger->info("充电机, disconnect !!! ");
    connected = false;

}

unsigned int modbus_crc16(unsigned char *p,unsigned int len)
{
    unsigned int wcrc=0XFFFF;//16位crc寄存器预置
    unsigned char temp;
    unsigned int i=0,j=0;//计数

    for(i=0;i<len;i++)//循环计算每个数据
    {
        temp=*p&0X00FF;//将八位数据与crc寄存器亦或
        p++;//指针地址增加，指向下个数据
        wcrc^=temp;//将数据存入crc寄存器
        for(j=0;j<8;j++)//循环计算数据的
        {
            if(wcrc&0X0001)//判断右移出的是不是1，如果是1则与多项式进行异或。
            {
                wcrc>>=1;//先将数据右移一位
                wcrc^=0XA001;//与上面的多项式进行异或
            }
            else//如果不是1，则直接移出
            {
                wcrc>>=1;//直接移出
            }
        }
    }
    return wcrc;//crc的值
}

bool chargemachine::chargeControl(unsigned char id, unsigned char type)
{   
    combined_logger->info("chargemachine{0}, chargeControl...{1}", id, type);
    std::unique_lock <std::mutex> lock(charging_mutex);

    std::vector<unsigned char> command;
    command.push_back(id);
    command.push_back(CHARGE_STATUS_QUERY);
    command.push_back(type);
    //voltage
    command.push_back(0x00);
    command.push_back(0x00);
    //I
    command.push_back(0x00);
    command.push_back(0x00);
    charging_finished = false;
    sendCommand(command.data(), command.size());
    //wait for chargingmachine告知充电状态
    combined_logger->info("chargemachine, wait for chargingmachine告知充电状态...");

    auto res = charging_status.wait_for(lock,std::chrono::milliseconds(timeout));
    if(cv_status::timeout == res)
    {
        combined_logger->info("chargemachine, 告知充电状态..., chargeControl timeout end");
        return false;
    }
    else
    {
        combined_logger->info("chargemachine, 告知充电状态..., chargeControl end");
        return true;
    }
}


bool chargemachine::chargeQuery(unsigned char id, unsigned char record_no)
{
    combined_logger->info("chargemachine{0}, chargeQuery...{1}", id, record_no);
    std::unique_lock <std::mutex> lock(query_mutex);

    std::vector<unsigned char> command;
    command.push_back(id);
    command.push_back(CHARGE_HISTORY_QUERY);
    command.push_back(record_no);
    command.push_back(0x00);
    command.push_back(0x00);
    command.push_back(0x00);
    command.push_back(0x00);
    sendCommand(command.data(), command.size());
    //wait for chargingmachine告知历史数据
    combined_logger->info("chargemachine, wait for chargingmachine告知历史数据...");

    auto res = charging_history.wait_for(lock, std::chrono::milliseconds(timeout));
    if(cv_status::timeout == res)
    {
        combined_logger->info("chargemachine, 告知历史数据..., chargeQuery timeout end");
        return false;
    }
    else
    {
        combined_logger->info("chargemachine, 告知历史数据..., chargeQuery end");
        return true;
    }
}
bool chargemachine::sendCommand(unsigned char*data, int len)
{
    unsigned int c = modbus_crc16(data, len);
    unsigned char CRC[2];
    CRC[0]=c;//crc的低八位
    CRC[1]=c>>8;//crc的高八位
    char* msg = new char[len+2];
    memcpy(msg, data, len);
    memcpy(msg+len, CRC, 2);
    bool ret = send(msg, len+2);
    delete msg;
    return ret;
}

bool check_crc(unsigned char* data, int len)
{
    unsigned int c = modbus_crc16(data, len-2);
    unsigned char CRC[2];
    CRC[0]=c;//crc的低八位
    CRC[1]=c>>8;//crc的高八位

    if(data[len-2] == CRC[0] && data[len-1] == CRC[1])
    {
        return true;
    }
    else
    {
        return false;
    }

}

void chargemachine::parse_real_data(const char *data,int len)
{
    if(len != 18)
    {
        combined_logger->error("充电机实时数据, data length error: "  + string(data));
        return;
    }
    //环境温度
    combined_logger->info("充电机, 环境温度: {0}"  , (int)data[3]);
    //实时电压
    u_short current_voltage = (u_short)((data[4]<<8)&0xFF00)+(u_short)(data[5]&0x00FF);
    combined_logger->info("充电机, 实时电压: {0}"  , current_voltage*10);
    //电池电压
    u_short battery_voltage = (u_short)((data[6]<<8)&0xFF00)+(u_short)(data[7]&0x00FF);
    combined_logger->info("充电机, 电池电压: {0}"  , battery_voltage*10);
    //充电电流
    u_short  charging_current  = (u_short)((data[8]<<8)&0xFF00)+(u_short)(data[9]&0x00FF);
    combined_logger->info("充电机, 充电电流: {0}"  , charging_current*10);
    //设定电压
    u_short  setting_voltage  = (u_short)((data[10]<<8)&0xFF00)+(u_short)(data[11]&0x00FF);
    combined_logger->info("充电机, 设定电压: {0}"  , setting_voltage*10);
    //设定电流
    u_short  setting_current  = (u_short)((data[12]<<8)&0xFF00)+(u_short)(data[13]&0x00FF);
    combined_logger->info("充电机, 设定电流: {0}"  , setting_current*10);
    //故障编号
    combined_logger->info("充电机, 故障编号: {0}"  , int(data[14]));
    //充电时间
    u_short  charge_time  = (u_short)((data[15]<<8)&0xFF00)+(u_short)(data[16]&0x00FF);
    combined_logger->info("充电机, 充电时间: {0}"  , charge_time);
    //充电锁定
    combined_logger->info("充电机, 充电锁定: {0}"  , int(data[16]));
    //充电状态
    combined_logger->info("充电机, 充电状态: {0}"  , int(data[17]));

    charging_status.notify_all();
}

void chargemachine::parse_history_data(const char *data,int len)
{
    if(len != 12)
    {
        combined_logger->error("充电机历史数据, data length error: "  + string(data));
        return;
    }
    //环境温度
    combined_logger->info("充电机, 环境温度: {0}"  , (int)data[3]);
    //起始电压
    u_short start_voltage = (u_short)((data[4]<<8)&0xFF00)+(u_short)(data[5]&0x00FF);
    combined_logger->info("充电机, 起始电压: {0}"  , start_voltage*10);
    //结束电压
    u_short end_voltage = (u_short)((data[6]<<8)&0xFF00)+(u_short)(data[7]&0x00FF);
    combined_logger->info("充电机, 结束电压: {0}"  , end_voltage*10);
    //充电时长
    u_short  charging_time  = (u_short)((data[8]<<8)&0xFF00)+(u_short)(data[9]&0x00FF);
    combined_logger->info("充电机, 充电时长: {0}"  , charging_time);
    //充电电量
    u_short  charging_power  = (u_short)((data[10]<<8)&0xFF00)+(u_short)(data[11]&0x00FF);
    combined_logger->info("充电机, 充电电量: {0}"  , charging_power*10);

    charging_history.notify_all();
}

void chargemachine::onRead(const char *data,int len)
{
    //combined_logger->info("充电机, data len: %d", len);
    std::cout<<"充电机, data len: "<< len <<std::endl;

    //crc_check
    if(!check_crc((unsigned char*)data, len))
    {
        std::cout<<"CRC校验未通过"<<std::endl;
        return;
    }


    switch (data[1]) {
    case CHARGE_STATUS_QUERY:
    {
        parse_real_data(data, len);
        break;
    }
    case CHARGE_HISTORY_QUERY:
    {
        parse_history_data(data, len);
        break;
    }
    default:
        combined_logger->error("充电机, data error: "  + string(data));
        break;
    }

}
