#include "chargemachine.h"
#include <limits>
chargemachine::chargemachine()
{

}
chargemachine::chargemachine(int _id, std::string _name, std::string _ip, int _port) :
    Device(_id,_name,_ip, _port)
{
    timeout = COMM_TIMEOUT;
    memset(&realData, 0x00, sizeof(realData));
}

void chargemachine::setParams(int _id, std::string _name, std::string _ip, int _port, int _timeout)
{
    id = _id;
    ip = _ip;
    port = _port;
    name = _name;
    timeout = _timeout;
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
    if(!checkConnection())
        return false;
    combined_logger->info("chargemachine{0}, chargeControl...{1}", id, type);
    std::unique_lock <std::mutex> lock(charging_mutex);

    std::vector<unsigned char> command;
    command.push_back(id);
    command.push_back(CHARGE_SET);
    command.push_back(type);
    //voltage
    command.push_back(0x00);
    command.push_back(0x00);
    //I
    command.push_back(0x00);
    command.push_back(0x00);
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

bool chargemachine::checkConnection()
{
    int reconnectTimes = 0;
    while((!connected) && (reconnectTimes < MAX_RECONNECT_TIMES))
    {
        if(!runFlag)
        {
            start();
        }
        else
        {
//            reconnect();
            reconnectTimes++;
        }
        sleep(3);
    }
    if(!connected)
        combined_logger->info("chargemachine, check connection error...");
    return connected;
}
bool chargemachine::chargeQueryList(unsigned char id, Json::Value &json)
{
    if(!checkConnection())
        return false;

    memset(&historyData, 0x00, sizeof(historyData));
    for(int record_no = 1; record_no <= MAX_RECORD_NUM; record_no++)
    {
        currentRecordID = record_no;
        chargeQuery(id, record_no);
    }

    Json::Value json_records;

    for(int record_no = 0; record_no < MAX_RECORD_NUM; record_no++)
    {
        Json::Value json_one_record;
        json_one_record["id"] = record_no;
        json_one_record["temperature"] = historyData[record_no].temperature;
        json_one_record["start_voltage"] = historyData[record_no].start_voltage;
        json_one_record["end_voltage"] = historyData[record_no].end_voltage;
        json_one_record["charge_time"] = historyData[record_no].charge_time;
        json_one_record["charge_power"] = historyData[record_no].charge_power;
        json_records.append(json_one_record);
    }
    json["records"] = json_records;
    return true;
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
    if(!connected)
    {
        combined_logger->info("wait for connect...");
    }
    unsigned int c = modbus_crc16(data, len);
    unsigned char CRC[2];
    CRC[0]=c;//crc的低八位
    CRC[1]=c>>8;//crc的高八位
    char* msg = new char[len+2];
    memcpy(msg, data, len);
    memcpy(msg+len, CRC, 2);
    bool ret = send(msg, len+2);
    combined_logger->info("sendresult:{0}...", ret);
    delete []msg;
    return ret;
}

bool check_crc(unsigned char* data, int len)
{
    unsigned int c = modbus_crc16(data, len-CRC_LEN);
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
    if(len != REAL_DATA_LEN)
    {
        combined_logger->error("充电机实时数据, data length error: "  + string(data));
        return;
    }
    //环境温度
    combined_logger->info("充电机, 环境温度: {0}"  , (int)data[3]);
    realData.temperature = data[3];
    //实时电压
    u_short current_voltage = (u_short)((data[4]<<8)&0xFF00)+(u_short)(data[5]&0x00FF);
    realData.real_voltage = current_voltage;
    combined_logger->info("充电机, 实时电压: {0}"  , current_voltage*10);
    //电池电压
    u_short battery_voltage = (u_short)((data[6]<<8)&0xFF00)+(u_short)(data[7]&0x00FF);
    realData.battery_voltage = battery_voltage;
    combined_logger->info("充电机, 电池电压: {0}"  , battery_voltage*10);
    //充电电流
    u_short  charging_current  = (u_short)((data[8]<<8)&0xFF00)+(u_short)(data[9]&0x00FF);
    realData.real_current = charging_current;
    combined_logger->info("充电机, 充电电流: {0}"  , charging_current*10);
    //设定电压
    u_short  setting_voltage  = (u_short)((data[10]<<8)&0xFF00)+(u_short)(data[11]&0x00FF);
    realData.set_voltage = setting_voltage;
    combined_logger->info("充电机, 设定电压: {0}"  , setting_voltage*10);
    //设定电流
    u_short  setting_current  = (u_short)((data[12]<<8)&0xFF00)+(u_short)(data[13]&0x00FF);
    realData.set_current = setting_current;
    combined_logger->info("充电机, 设定电流: {0}"  , setting_current*10);
    //故障编号
    realData.faultcode.Data = data[14];
    combined_logger->info("充电机, 故障编号: {0}{1}{2}{3}{4}{5}{6}{7}"  , (data[14]&0x80)>>7,(data[14]&0x40)>>6,(data[14]&0x20)>>5,(data[14]&0x10)>>4,(data[14]&0x08)>>3, (data[14]&0x04)>>2, (data[14]&0x02)>>1, data[14]&0x0001 );
    //充电时间
    u_short  charge_time  = (u_short)((data[15]<<8)&0xFF00)+(u_short)(data[16]&0x00FF);
    realData.charge_time = charge_time;
    combined_logger->info("充电机, 充电时间: {0}"  , charge_time);
    //充电锁定
    combined_logger->info("充电机, 充电锁定: {0}"  , int(data[17]));
    realData.charge_lock = data[17];
    //充电状态
    combined_logger->info("充电机, 充电状态: {0}"  , int(data[18]));
    realData.charge_status = (CHARGE_STATUS)data[18];
    std::unique_lock <std::mutex> unlock(charging_mutex);
    charging_status.notify_all();
}

void chargemachine::parse_history_data(const char *data,int len)
{
    if(len != HISTORY_DATA_LEN)
    {
        combined_logger->error("充电机历史数据, data length error: "  + string(data));
        return;
    }
    //环境温度
    historyData[currentRecordID-1].temperature = data[3];
    combined_logger->info("充电机, 环境温度: {0}"  , (int)data[3]);
    //起始电压
    historyData[currentRecordID-1].start_voltage = (u_short)((data[4]<<8)&0xFF00)+(u_short)(data[5]&0x00FF);
    combined_logger->info("充电机, 起始电压: {0}"  , historyData[currentRecordID-1].start_voltage*10);
    //结束电压
    historyData[currentRecordID-1].end_voltage = (u_short)((data[6]<<8)&0xFF00)+(u_short)(data[7]&0x00FF);
    combined_logger->info("充电机, 结束电压: {0}"  , historyData[currentRecordID-1].end_voltage*10);
    //充电时长
    historyData[currentRecordID-1].charge_time  = (u_short)((data[8]<<8)&0xFF00)+(u_short)(data[9]&0x00FF);
    combined_logger->info("充电机, 充电时长: {0}"  , historyData[currentRecordID-1].charge_time);
    //充电电量
    historyData[currentRecordID-1].charge_power  = (u_short)((data[10]<<8)&0xFF00)+(u_short)(data[11]&0x00FF);
    combined_logger->info("充电机, 充电电量: {0}"  , historyData[currentRecordID-1].charge_power*10);

    std::unique_lock <std::mutex> unlock(query_mutex);
    charging_history.notify_all();
}

void chargemachine::onRead(const char *data,int len)
{
    combined_logger->info("充电机, data len: {0}, buffer size:{1}", len, buffer.size());
    buffer.append(data,len);

    int msg_len = INT_MAX;
    if(buffer.size() > MSG_HEAD_LEN)
    {
        msg_len = buffer.buf[2]&0x000000FF;
        msg_len += (MSG_HEAD_LEN+CRC_LEN);
    }

    if(msg_len > buffer.size())
    {
        //wait for message
        return;
    }
    while(msg_len <= buffer.size())
    {
        //crc_check
        if(!check_crc((unsigned char*)buffer.buf.data(), msg_len))
        {
            std::cout<<"CRC校验未通过"<<std::endl;
            buffer.removeFront(msg_len);

            if(buffer.size() > MSG_HEAD_LEN)
            {
                msg_len = buffer.buf[2]&0x000000FF;
                msg_len += (MSG_HEAD_LEN+CRC_LEN);
                continue;
            }
            else
            {
                return;
            }
        }
        switch (buffer.buf[1]) {
        case CHARGE_STATUS_QUERY:
        {
            parse_real_data(buffer.buf.data(), msg_len);
            break;
        }
        case CHARGE_HISTORY_QUERY:
        {
            parse_history_data(buffer.buf.data(), msg_len);
            break;
        }
        default:
            combined_logger->error("充电机, data error: "  + string(data));
            break;
        }
        buffer.removeFront(msg_len);
        msg_len = INT_MAX;
        if(buffer.size() > MSG_HEAD_LEN)
        {
            msg_len = buffer.buf[2]&0x000000FF;
            msg_len += (MSG_HEAD_LEN+CRC_LEN);
        }
    }
}
