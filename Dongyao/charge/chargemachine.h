#ifndef CHARGEMACHINE_H
#define CHARGEMACHINE_H
#include "../../device/device.h"

enum CHARGE_QUERY
{
  CHARGE_STATUS_QUERY = 0x03,
  CHARGE_HISTORY_QUERY= 0x07
};

enum CHARGE_COMMAND
{
  CHARGE_START = 0x01,
  CHARGE_STOP= 0x03
};

//充电机client class
class chargemachine : public Device
{
public:

    chargemachine(int id,std::string name,std::string ip, int port);
    virtual ~chargemachine();

    bool chargeControl(unsigned char id, unsigned char type);
    bool chargeQuery(unsigned char id, unsigned char record_no);

    bool isChargingFinished()
    {
        return charging_finished;
    }

private:

    void onRead(const char *data,int len);
    void onConnect();
    void onDisconnect();
    bool sendCommand(unsigned char *data, int len);
    void parse_real_data(const char *data,int len);
    void parse_history_data(const char *data,int len);

    std::mutex charging_mutex, query_mutex;
    bool charging_finished;
    int timeout;
    std::condition_variable charging_status; // charging_status条件变量.
    std::condition_variable charging_history;

};



#endif // CHARGEMACHINE_H
