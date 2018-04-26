#ifndef AGVSTATION_H
#define AGVSTATION_H
#include <string>
#include <memory>

class AgvStation;
using AgvStationPtr = std::shared_ptr<AgvStation>;

class Agv;
using AgvPtr = std::shared_ptr<Agv>;

//Agv可到达和停留的站点
class AgvStation : public std::enable_shared_from_this<AgvStation>
{
public:

    //point type表示这个点，agv是否能够停留
    typedef enum _station_type{
        STATION_TYPE_DRAW = 0,//只是提供绘图的时候的点//不可停留点
        STATION_TYPE_REPORT,//AGV到达报告点(包括以下的点，都需要报告)
        STATION_TYPE_HALT,//可以停留的点
        STATION_TYPE_CHARGE,//充点电
        STATION_TYPE_LOAD,//上货点
        STATION_TYPE_UNLOAD,//卸货点
        STATION_TYPE_LOAD_UNLOAD,//上货、卸货点
    }STATION_TYPE;

    AgvStation():
        x(0),
        y(0),
        name(""),
        id(0),
        occuAgv(nullptr),
        mapChangeStation(false),
        floorId(0),
        stationType(STATION_TYPE_HALT),
        realX(0),
        realY(0),
        labelXoffset(0),
        labelYoffset(0),
        locked(false)
    {

    }

    AgvStation(const AgvStation &b)
    {
        x = b.x;
        y = b.y;
        name = b.name;
        id = b.id;
        occuAgv = b.occuAgv;
        mapChangeStation = b.mapChangeStation;
        floorId = b.floorId;
        stationType = b.stationType;
        realX = b.realX;
        realY = b.realY;
        labelXoffset = b.labelXoffset;
        labelYoffset = b.labelYoffset;
        locked = b.locked;
    }

    bool operator <(const AgvStation &b) {
        return id<b.id;
    }
    bool operator == (const AgvStation &b) {
        return this->id == b.id;
    }

    int getId(){return id;}
    std::string getName(){return name;}
    int getX(){return x;}
    int getY(){return y;}
    int getFloorId(){return floorId;}
    STATION_TYPE getStationType(){return stationType;}
    AgvPtr getOccuAgv(){return occuAgv;}
    bool getMapChangeStation(){return mapChangeStation;}
    int getRealX(){return realX;}
    int getRealY(){return realY;}
    int getLabelXoffset(){return labelXoffset;}
    int getLabelYoffset(){return labelYoffset;}
    bool getLocked(){return locked;}

    void setId(int _id){id =_id ;}
    void setName(std::string _name){ name= _name ;}
    void setX(int _x){x = _x ;}
    void setY(int _y){y = _y ;}
    void setFloorId(int _floorId){floorId = _floorId ;}
    void setStationtype(STATION_TYPE _stationType){stationType = _stationType ;}
    void setOccuAgv(AgvPtr _occuAgv){occuAgv = _occuAgv ;}
    void setMapChangeStation(bool _mapChangeStation){mapChangeStation = _mapChangeStation ;}
    void setRealX(int _realX){realX = _realX ;}
    void setRealY(int _realY){realY = _realY ;}
    void setLabelXoffset(int _labelXoffset){labelXoffset = _labelXoffset ;}
    void setLabelYoffset(int _labelYoffset){labelYoffset = _labelYoffset ;}
    void setLocked(bool _locked){locked = _locked ;}


protected:
    int id;
    std::string name;
    int x;//显示地图中的坐标
    int y;//显示地图中的坐标
    int floorId;//所属地图的ID
    STATION_TYPE stationType;//点类型
    AgvPtr occuAgv;//占用的AGV
    bool mapChangeStation;//地图切换点
    int realX;//映射到机器人地图中的点位信息
    int realY;//映射到机器人地图中的点位信息
    int labelXoffset;//标签的X偏移
    int labelYoffset;//标签的Y偏移
    bool locked;//是否被交通管制了
};

#endif // AGVSTATION_H
