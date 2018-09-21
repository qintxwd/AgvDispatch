#include "qingdaonodetingmove.h"

QingdaoNodeTingMove::QingdaoNodeTingMove(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{
}


QingdaoNodeTingMove::~QingdaoNodeTingMove()
{
}

void QingdaoNodeTingMove::beforeDoing(AgvPtr agv)
{

}

void QingdaoNodeTingMove::doing(AgvPtr agv)
{
    bresult = true;
}


void QingdaoNodeTingMove::afterDoing(AgvPtr agv)
{
}

bool QingdaoNodeTingMove::result(AgvPtr agv)
{
    return bresult;
}
