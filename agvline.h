#ifndef AGVLINE_H
#define AGVLINE_H
#include <vector>
#include <memory>

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

class AgvLine;
using AgvLinePtr = std::shared_ptr<AgvLine>;
//站点之间的连线
class AgvLine : public std::enable_shared_from_this<AgvLine>
{
public:
    AgvLine():
        id(0),
        length(0),
        father(0),
        distance(0),
        color(0),
        name(""),
        floorId(0)
    {

    }

    AgvLine(const AgvLine &b)
    {
        id = b.id;
        name = b.name;
        length = b.length;
        startStation = b.startStation;
        endStation = b.endStation;
        name = b.name;
        locked = b.locked;
        occuAgvs = b.occuAgvs;
        floorId = b.floorId;
    }

    bool operator <(const AgvLine &b) {
        return id<b.id;
    }

    bool operator == (const AgvLine &b) {
        return this->startStation == b.startStation&& this->endStation == b.endStation;
    }

    int getId(){return id;}
    std::string getName(){return name;}
    AgvStationPtr getStartStation(){return startStation;}
    AgvStationPtr getEndStation(){return endStation;}
    int getLength(){return length;}
    std::vector<AgvPtr> getOccuAgvs(){return occuAgvs;}
    bool getLocked(){return locked;}
    int getFloorId(){return floorId;}

    void setId(int _id){id =_id ;}
    void setName(std::string _name){ name= _name ;}
    void setStartStation(AgvStationPtr _startStation){startStation=_startStation;}
    void setEndStation(AgvStationPtr _endStation){endStation=_endStation;}
    void setLength( int _length){length=_length;}
    void setOccuAgvs( std::vector<AgvPtr> _occuAgvs){occuAgvs=_occuAgvs;}
    void setLocked( bool _locked){locked=_locked;}
    void setFloorId(int _floorId){floorId = _floorId ;}

private:
    std::vector<AgvPtr> occuAgvs;
    int id;
    std::string name;
    int length;
    AgvStationPtr startStation = nullptr;//起始站点
    AgvStationPtr endStation = nullptr;//终止站点
    bool locked;//是否被交通管制
    int floorId;//所属地图的ID
public:
    //计算路径用的
    AgvLinePtr father;
    int distance;//起点到这条线的终点 的距离
    int color;

};

#endif // AGVLINE_H
