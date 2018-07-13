#ifndef DYMAPPATH
#define DYMAPPATH
#include "../mapmap/mappoint.h"

class DyMapPath : public MapSpirit
{
public:
    enum Map_DyPath_Type{
        Map_Path_Type_Line = 1,
        Map_Path_Type_Bezier = 2,
        Map_Path_Type_Between_Floor =3//楼层间线路
    };

    DyMapPath(int _id, std::string _name, int _start, int _end, Map_DyPath_Type _type, int _p1x = 0, int _p1y = 0, int _p1a = 0, int _p1f = 1, int _p2x=0, int _p2y = 0, int _p2a = 0, int _p2f = 1, float _speed=0.2);
    virtual ~DyMapPath();
    virtual MapSpirit *clone();
    DyMapPath(const DyMapPath &p) = delete;

    void setPathType(Map_DyPath_Type _path_type){path_type=_path_type;}
    void setStart(int _start){start = _start;}
    void setEnd(int _end) { end = _end; }
    void setP1x(int _p1x) { p1x = _p1x; }
    void setP1y(int _p1y) { p1y = _p1y; }
    void setP1a(int _p1a) { p1a = _p1a; }
    void setP1f(int _p1f) { p1f = _p1f; }
    void setP2x(int _p2x) { p2x = _p2x; }
    void setP2y(int _p2y) { p2y = _p2y; }
    void setP2a(int _p2a) { p2a = _p2a; }
    void setP2f(int _p2f) { p2f = _p2f; }
    void setSpeed(float _speed) { speed = _speed; }


    //void setDirection(int _direction) { direction = _direction; }

    int getStart() const {return start;}
    int getEnd() const {return end;}
    int getP1x() const {return p1x;}
    int getP1y() const {return p1y;}
    int getP1a() const {return p1a;}
    int getP1f() const {return p1f;}
    int getP2x() const {return p2x;}
    int getP2y() const {return p2y;}
    int getP2a() const {return p2a;}
    int getP2f() const {return p2f;}
    float getSpeed() const {return speed;}

    //int getDirection() const {return direction;}// 0 (双向)  1(start-->end)  2(end-->start)
    Map_DyPath_Type getPathType() const { return path_type; }
private:
    Map_DyPath_Type path_type;
    int start;
    int end;
    int p1x;
    int p2x;
    int p1a;
    int p2a;
    int p1y;
    int p2y;
    int p1f;
    int p2f;
    float speed;
};
#endif // DYMAPPATH

