#ifndef AGVLINE_H
#define AGVLINE_H
#include <vector>
#include <memory>
#include "onemap.h"

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

class AgvLine;
using AgvLinePtr = std::shared_ptr<AgvLine>;
//站点之间的连线
class AgvLine : public std::enable_shared_from_this<AgvLine>,public MapPath
{
public:
    AgvLine(int _id, std::string _name, int _start, int _end, Map_Path_Type _type, int _length, int _p1x = 0, int _p1y = 0, int _p2x = 0, int _p2y = 0, bool _locked = false):
        MapPath(_id,_name,_start,_end,_type,_length,_p1x,_p1y,_p2x,_p2y,_locked),
        father(0),
        distance(0),
        color(0)
    {

    }

    bool operator == (const AgvLine &b) {
		int id = getId();
		int _id = b.getId();
		return id == _id;
    }

    std::vector<int> getOccuAgvs(){return occuAgvs;}
    void setOccuAgvs( std::vector<int> _occuAgvs){occuAgvs=_occuAgvs;}
private:
    std::vector<int> occuAgvs;
public:
    //计算路径用的
	AgvLinePtr father;
    int distance;//起点到这条线的终点 的距离
    int color;

};

#endif // AGVLINE_H
