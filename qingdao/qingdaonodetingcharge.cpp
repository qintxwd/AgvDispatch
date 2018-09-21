#include "qingdaonodetingcharge.h"
#include "../common.h"


QingdaoNodeTingCharge::QingdaoNodeTingCharge(std::vector<std::string> _params):
	AgvTaskNodeDoThing(_params)
{
}


QingdaoNodeTingCharge::~QingdaoNodeTingCharge()
{
}

void QingdaoNodeTingCharge::beforeDoing(AgvPtr agv)
{

}

void QingdaoNodeTingCharge::doing(AgvPtr agv)
{
	Sleep(1500);
	bresult = true;
}


void QingdaoNodeTingCharge::afterDoing(AgvPtr agv)
{
}

bool QingdaoNodeTingCharge::result(AgvPtr agv)
{
	return bresult;
}
