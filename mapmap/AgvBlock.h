#pragma once
#include <vector>
#include <mutex>
#include "onemap.h"

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

class AgvLine;
using AgvLinePtr = std::shared_ptr<AgvLine>;

class AgvBlock;
using AgvBlockPtr = std::shared_ptr<AgvBlock>;

//一个区块内最多只能同时允许一辆车
class AgvBlock: public std::enable_shared_from_this<AgvBlock>, public MapBlock
{
public:
	AgvBlock(int _id, std::string _name);
	virtual ~AgvBlock();

	bool setAgv(AgvPtr agv);
	AgvPtr getAgv();
	void setNullAgv();

private:
	AgvPtr currentAgv;
	std::mutex mtx;
};

