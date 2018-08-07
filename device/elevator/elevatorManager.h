#ifndef ELEVATORMANAGER_H
#define ELEVATORMANAGER_H

#include <vector>
#include <memory>
#include <mutex>
#include "utils/noncopyable.h"
#include "protocol.h"
#include "network/tcpsession.h"
#include "elevator.h"

class Elevator;
using ElevatorPtr = std::shared_ptr<Elevator>;

class ElevatorManager;
using ElevatorManagerPtr = std::shared_ptr<ElevatorManager>;

class ElevatorManager: public noncopyable,public std::enable_shared_from_this<ElevatorManager>
{
public:
    bool init();

    static ElevatorManagerPtr getInstance(){
        static ElevatorManagerPtr m_ins = ElevatorManagerPtr(new ElevatorManager());
        return m_ins;
    }

    ElevatorPtr getElevatorById(int id);

public:
    ElevatorManager();
    ~ElevatorManager();
private:
    void checkTable();

    std::mutex mtx;
    std::vector<ElevatorPtr> elevators;
};

#endif // ELEVATORMANAGER_H

