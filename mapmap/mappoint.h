#ifndef MAPPOINT_H
#define MAPPOINT_H
#include "mapspirit.h"


class MapPoint : public MapSpirit
{
public:
    enum Map_Point_Type{
        Map_Point_Type_Draw = 0,//绘画点，无实际点
        Map_Point_Type_REPORT,//报高点，不可停留点【多数是这种点】
        Map_Point_Type_HALT,//躲避点
        Map_Point_Type_CHARGE,//充点电
        Map_Point_Type_LOAD,//上货点
        Map_Point_Type_UNLOAD,//下货点
        Map_Point_Type_LOAD_UNLOAD,//上下货点【多数上下货的点是这种点】
        Map_Point_Type_ORIGIN,//原点
        Map_Point_Type_NO_UTURN
    };

    MapPoint(int _id, std::string _name,Map_Point_Type _type, int _x, int _y,int _realX = 0,int _realY = 0,int _realA = 0,int _labelXoffset = 0,int _labelYoffset = -40,bool _mapChange = false,bool _locked = false, std::string _ip = "", int _port = 0, int _agvType = -1, std::string _lineId = "");
    virtual ~MapPoint();
    virtual MapSpirit *clone();
    MapPoint(const MapPoint &p) = delete;

    void setPointType(Map_Point_Type _point_type){point_type=_point_type;}
    Map_Point_Type getPointType() const {return point_type;}
    int getX() const {return x;}
    int getY() const {return y;}
    int getRealX() const {return realX;}
    int getRealY() const {return realY;}
    int getRealA() const {return realA;}
    int getLabelXoffset() const {return labelXoffset;}
    int getLabelYoffset() const {return labelYoffset;}
    bool getMapChange() const {return mapChange;}
    bool getLocked() const {return locked;}
    std::string getIp(){return ip;}
    int getPort(){return port;}
    int getAgvType(){return agvType;}
    std::string getLineId(){return lineId;}

    void setRealX(int _realX){realX = _realX ;}
    void setRealY(int _realY){realY = _realY ;}
    void setRealA(int _realA){realA = _realA;}
    void setLabelXoffset(int _labelXoffset){labelXoffset = _labelXoffset ;}
    void setLabelYoffset(int _labelYoffset){labelYoffset = _labelYoffset ;}
    void setX(int _x){x = _x ;}
    void setY(int _y){y = _y ;}
    void setMapChange(bool _mapChange){mapChange = _mapChange;}
    void setLocked(bool _locked){locked = _locked;}
    void setIp(std::string _ip){ip = _ip;}
    void setPort(int _port){port = _port ;}
    void setAgvType(int _agvType){agvType = _agvType ;}
    void setLineId(std::string _lineId){lineId = _lineId;}

private:
    Map_Point_Type point_type;
    int x;
    int y;
    int realX;
    int realY;
    int realA;
    int labelXoffset;
    int labelYoffset;
    bool mapChange;
    bool locked;
    std::string ip;//对接设备ip, 无需对接则为 ""
    int port; //对接设备port, 无需对接则为 0
    int agvType;//和设备对接的AGV类型, 如果项目只有一种类型AGV, 未设置缺省为-1,可以忽略,
    std::string lineId; //产线ID, 没有可以忽略
};

#endif // MAPPOINT_H
