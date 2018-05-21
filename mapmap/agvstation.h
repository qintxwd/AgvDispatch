#ifndef AGVSTATION_H
#define AGVSTATION_H
#include <string>
#include <memory>
#include "onemap.h"

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

//Agv可到达和停留的站点
class AgvStation : public std::enable_shared_from_this<AgvStation>, public MapPoint
{
public:

	AgvStation(int _id, std::string _name, Map_Point_Type _type, int _x, int _y, int _realX = 0, int _realY = 0, int _labelXoffset = 0, int _labelYoffset = -40, bool _mapChange = false, bool _locked = false) :
		MapPoint(_id, _name, _type, _x, _y, _realX, _realY, _labelXoffset, _labelYoffset, _mapChange, _locked),
		occuAgv(0)
	{
	}

	int getOccuAgv() { return occuAgv; }
	void setOccuAgv(int _occuAgv) { occuAgv = _occuAgv; }
protected:
	int occuAgv;//占用的AGV
};

#endif // AGVSTATION_H
