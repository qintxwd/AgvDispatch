#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include "utils/noncopyable.h"
#include "protocol.h"
#include "network/session.h"

enum DEVICE_TYPE
{
    DEVICE_CHARGEMACHINE = 3
};

class Device;
using DevicePtr = std::shared_ptr<Device>;

class DeviceManager;
using DeviceManagerPtr = std::shared_ptr<DeviceManager>;

class DeviceManager: public noncopyable,public std::enable_shared_from_this<DeviceManager>
{
public:
    typedef std::function< void(DevicePtr) >  DeviceEachCallback;

    bool init();

    static DeviceManagerPtr getInstance(){
        static DeviceManagerPtr m_ins = DeviceManagerPtr(new DeviceManager());
        return m_ins;
    }

    DevicePtr getDeviceById(int id, int type);

    DevicePtr getDeviceByIP(std::string ip);

    void foreachDevice(DeviceEachCallback cb);

    //用户接口
    void getDeviceLog(SessionPtr conn, const Json::Value &request);
    void interElevatorControl(SessionPtr conn, const Json::Value &request);
protected:
    DeviceManager();
private:

    std::mutex mtx;
    std::vector<DevicePtr> devices;
};

#endif // DEVICEMANAGER_H
