#include "AgvBlock.h"
#include "../agv.h"
#include "agvstation.h"
#include "agvline.h"
#include "../common.h"

AgvBlock::AgvBlock(int _id, std::string _name):
	MapBlock(_id,_name)
{
}

AgvBlock::~AgvBlock()
{
}

bool AgvBlock::setAgv(AgvPtr agv)
{
	UNIQUE_LCK(mtx);
	if (currentAgv != nullptr)return false;
	currentAgv = agv;
	return true;
}

AgvPtr AgvBlock::getAgv()
{
	UNIQUE_LCK(mtx);
	return currentAgv;
}

void AgvBlock::setNullAgv()
{
	UNIQUE_LCK(mtx);
	currentAgv = nullptr;
}
