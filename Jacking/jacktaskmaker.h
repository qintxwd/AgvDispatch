#ifndef JACKTASKMAKER_H
#define JACKTASKMAKER_H

#include "taskmaker.h"

class JackTaskMaker : public TaskMaker
{
public:
    JackTaskMaker();

    void makeTask(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
};

#endif // JACKTASKMAKER_H
