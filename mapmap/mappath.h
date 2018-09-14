#ifndef MAPPATH_H
#define MAPPATH_H

#include "mappoint.h"


class MapPath : public MapSpirit
{
public:
    enum Map_Path_Type{
        Map_Path_Type_Line = 0,
        Map_Path_Type_Quadratic_Bezier,
        Map_Path_Type_Cubic_Bezier,
        Map_Path_Type_Between_Floor,//楼层间线路
    };

    MapPath(int _id, std::string _name, int _start, int _end, Map_Path_Type _type, int _length, int _p1x = 0, int _p1y = 0, int _p2x=0, int _p2y = 0, bool _locked = false, double _speed = 0.3);
    virtual ~MapPath();
    virtual MapSpirit *clone();
    MapPath(const MapPath &p) = delete;

    void setPathType(Map_Path_Type _path_type){path_type=_path_type;}
    void setStart(int _start){start = _start;}
	void setEnd(int _end) { end = _end; }
	void setP1x(int _p1x) { p1x = _p1x; }
	void setP1y(int _p1y) { p1y = _p1y; }
	void setP2x(int _p2x) { p2x = _p2x; }
	void setP2y(int _p2y) { p2y = _p2y; }
	void setLength(int _length) { length = _length; }
	void setLocked(bool _locked) { locked = _locked; }
    void setSpeed(double _speed) { speed = _speed; }

    int getStart() const {return start;}
    int getEnd() const {return end;}
    int getP1x() const {return p1x;}
    int getP1y() const {return p1y;}
    int getP2x() const {return p2x;}
    int getP2y() const {return p2y;}
    int getLength() const {return length;}
    bool getLocked() const {return locked;}
    double getSpeed() const {return speed;}

	Map_Path_Type getPathType() const { return path_type; }
private:
    Map_Path_Type path_type;
    int start;
    int end;
    int p1x;
    int p2x;
    int p1y;
    int p2y;
    int length;
    bool locked;
    double speed;
};

#endif // MAPPATH_H
