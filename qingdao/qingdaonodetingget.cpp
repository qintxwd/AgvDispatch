#include "qingdaonodetingget.h"
#include "../common.h"


QingdaoNodeTingGet::QingdaoNodeTingGet(std::vector<std::string> _params):
	AgvTaskNodeDoThing(_params),
    bresult(false)
{
}


QingdaoNodeTingGet::~QingdaoNodeTingGet()
{
}

void QingdaoNodeTingGet::beforeDoing(AgvPtr agv)
{

}

void QingdaoNodeTingGet::doing(AgvPtr agv)
{
	//≤‚ ‘ 

	Sleep(15000);
	bresult = true;
}


void QingdaoNodeTingGet::afterDoing(AgvPtr agv)
{
}

bool QingdaoNodeTingGet::result(AgvPtr agv)
{
	return bresult;
}
