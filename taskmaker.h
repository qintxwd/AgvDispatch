#ifndef TASKMAKER_H
#define TASKMAKER_H

#include "network/tcpsession.h"
#include "protocol.h"

class TaskMaker
{
public:
    static  TaskMaker* getInstance();
    virtual ~TaskMaker();
    virtual void makeTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
protected:
    TaskMaker();
};

#endif // TASKMAKER_H
