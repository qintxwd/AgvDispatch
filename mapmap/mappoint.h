#ifndef MAPPOINT_H
#define MAPPOINT_H
#include "mapspirit.h"


class MapPoint : public MapSpirit
{
public:
    enum Map_Point_Type{
        Map_Point_Type_Draw = 0,
        Map_Point_Type_REPORT,
        Map_Point_Type_HALT,
        Map_Point_Type_CHARGE,
        Map_Point_Type_LOAD,
        Map_Point_Type_UNLOAD,
        Map_Point_Type_LOAD_UNLOAD,
    };

    MapPoint(int _id, std::string _name,Map_Point_Type _type, int _x, int _y,int _realX = 0,int _realY = 0,int _labelXoffset = 0,int _labelYoffset = -40,bool _mapChange = false,bool _locked = false);

    MapPoint(const MapPoint &p);

    void setPointType(Map_Point_Type _point_type){point_type=_point_type;}
    Map_Point_Type getPointType(){return point_type;}
    int getX(){return x;}
    int getY(){return y;}
    int getRealX(){return realX;}
    int getRealY(){return realY;}
    int getLabelXoffset(){return labelXoffset;}
    int getLabelYoffset(){return labelYoffset;}
    bool getMapChange(){return mapChange;}
    bool getLocked(){return locked;}

    void setRealX(int _realX){realX = _realX ;}
    void setRealY(int _realY){realY = _realY ;}
    void setLabelXoffset(int _labelXoffset){labelXoffset = _labelXoffset ;}
    void setLabelYoffset(int _labelYoffset){labelYoffset = _labelYoffset ;}
    void setX(int _x){x = _x ;}
    void setY(int _y){y = _y ;}
    void setMapChange(bool _mapChange){mapChange = _mapChange;}
    void setLocked(bool _locked){locked = _locked;}
private:
    Map_Point_Type point_type;
    int x;
    int y;
    int realX;
    int realY;
    int labelXoffset;
    int labelYoffset;
    bool mapChange;
    bool locked;
};

#endif // MAPPOINT_H
