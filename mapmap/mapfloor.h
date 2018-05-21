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

    void removePoint(int p){points.remove(p);}
    void removePath(int p){paths.remove(p);}
    void removeBkg(){bkg = 0;}

    MapPoint *getPointById(int id) const;

    MapPath *getPathById (int id)  const;

    std::list<int> getPoints() const {return points;}
    std::list<int> getPaths() const {return paths;}
    int getBkg() const {return bkg;}

private:
    std::list<int> points;///若干站点
    std::list<int> paths;///若干路径
    int bkg;///一个背景图片
};

#endif // MAPFLOOR_H
