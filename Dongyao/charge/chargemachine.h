#ifndef CHARGEMACHINE_H
#define CHARGEMACHINE_H
#include "../../device/device.h"
#include "qyhbuffer.h"
#define MAX_RECORD_NUM 20
#define MSG_HEAD_LEN 3
#define REAL_DATA_LEN 21
#define HISTORY_DATA_LEN 14
#define CRC_LEN 2
#define COMM_TIMEOUT 500
#define MAX_RECONNECT_TIMES 3
enum CHARGE_QUERY
{
    CHARGE_STATUS_QUERY = 0x03,
    CHARGE_SET = 0x05,
    CHARGE_HISTORY_QUERY= 0x07
};

enum CHARGE_COMMAND
{
    CHARGE_START = 0x01,
    CHARGE_STOP= 0x03
};

enum CHARGE_STATUS
{
    CHARGE_IDLE = 0x00,
    CHARGE_ING= 0x02,
    CHARGE_FULL = 0x03
};

typedef struct
{
    unsigned char retracting:1;
    unsigned char ready:1;
    unsigned char stretching:1;
    unsigned char noBattery:1;
    unsigned char stretchError:1;
    unsigned char overVoltage:1;
    unsigned char overCurrent:1;
    unsigned char moduleError:1;
}FAULT_BIT;

typedef union
{
    FAULT_BIT Bit;
    unsigned char Data;
}FAULT_STATUS;

typedef struct
{
    unsigned char address;//1 byte
    unsigned char cid;//1 byte
    unsigned char len;//1 byte
    unsigned char temperature;//1 byte
    unsigned short real_voltage;//2 byte
    unsigned short battery_voltage;//2 byte
    unsigned short real_current;//2 byte
    unsigned short set_voltage;//2 byte
    unsigned short set_current;//2 byte
    FAULT_STATUS faultcode;//1 byte
    unsigned short charge_time;//2 byte
    unsigned char charge_lock;//1 byte
    CHARGE_STATUS charge_status;//8 bit
    unsigned short crc;//2 byte
}REALDATA_RECEIVE;


typedef struct
{
    unsigned char temperature;//1 byte
    unsigned short start_voltage;//2 byte
    unsigned short end_voltage;//2 byte
    unsigned short charge_time;//2 byte
    unsigned short charge_power;//2 byte
}HISTORYDATA_RECEIVE;

class chargemachine;
using chargemachinePtr = std::shared_ptr<chargemachine>;

//充电机client class
class chargemachine : public Device
{
public:

    chargemachine();
    chargemachine(int id,std::string name,std::string ip, int port);
    virtual ~chargemachine();

    bool chargeControl(unsigned char id, unsigned char type);
    bool chargeQuery(unsigned char id, unsigned char record_no);
    bool chargeQueryList(unsigned char id, Json::Value &json);
    void setParams(int id, string _name, std::string ip, int port, int _timeout);
    bool checkConnection();
    void setStationID(int id){stationID = id;}
    int getStatus()
    {
        return realData.charge_status;
    }
    bool getReady()
    {
        return realData.faultcode.Bit.ready;
    }

private:

    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();
    bool sendCommand(unsigned char *data, int len);
    void parse_real_data(const char *data,int len);
    void parse_history_data(const char *data,int len);
    std::mutex charging_mutex, query_mutex;
    int timeout;
    std::condition_variable charging_status; // charging_status条件变量.
    std::condition_variable charging_history;
    REALDATA_RECEIVE realData;
    HISTORYDATA_RECEIVE historyData[MAX_RECORD_NUM];
    QyhBuffer buffer;
    int stationID;
    int currentRecordID;
};



#endif // CHARGEMACHINE_H
