#ifndef MAPFLOOR_H
#define MAPFLOOR_H

#include <list>
#include "mappath.h"
#include "mapbackground.h"
#include "mappoint.h"

class OneMap;

class MapFloor : public MapSpirit
{
public:
    MapFloor(int _id, std::string _name);

    virtual ~MapFloor();
    virtual MapSpirit *clone() ;
    MapFloor(const MapFloor& b) = delete;

    void addPoint(int p){points.push_back(p);}
    void addPath(int p){paths.push_back(p);}
    void setBkg(int b){bkg = b;}
    void setOriginX(int _originX){originX = _originX;}
    void setOriginY(int _originY){originY = _originY;}
    void setRate(double _rate){rate = _rate;}
    void setOriginTheta(int _originTheta){originTheta=_originTheta;}

    void removePoint(int p){points.remove(p);}
    void removePath(int p){paths.remove(p);}
    void removeBkg(){bkg = 0;}

    MapPoint *getPointById(int id) const;

    MapPath *getPathById (int id)  const;

    std::list<int> getPoints() const {return points;}
    std::list<int> getPaths() const {return paths;}
    int getBkg() const {return bkg;}
    int getOriginX(){return originX;}
    int getOriginY(){return originY;}
    double getRate(){return rate;}
    int getOriginTheta(){return originTheta;}
private:
    std::list<int> points;///若干站点
    std::list<int> paths;///若干路径
    int bkg;///一个背景图片
    int originX;//原点位置的 对应位置
    int originY;//原点位置的 对应位置
    double rate;//agv的距离（实际距离） 和 地图位置的比例
    int originTheta;//原点位置 对应的角度
};

#endif // MAPFLOOR_H
