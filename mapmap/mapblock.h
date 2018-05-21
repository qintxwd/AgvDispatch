#ifndef MAPBLOCK_H
#define MAPBLOCK_H

#include <list>
#include "mapspirit.h"

//一个block内同时只允许一个AGV
class MapBlock : public MapSpirit
{
public:
    explicit MapBlock(int _id, std::string _name);
    virtual ~MapBlock();
    virtual MapSpirit *clone() ;
    MapBlock(const MapBlock& b) = delete;

    void addSpirit(int spirit){spirits.push_back(spirit);}
    void removeSpirit(int spirit){
        spirits.remove(spirit);
    }
    void clear(){spirits.clear();}
    std::list<int> getSpirits() const {return spirits;}
private:
    std::list<int> spirits;
};

#endif // MAPBLOCK_H
