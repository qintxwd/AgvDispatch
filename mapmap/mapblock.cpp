#include "mapblock.h"

MapBlock::MapBlock(int _id, std::string _name) :
    MapSpirit(_id,_name,Map_Sprite_Type_Block)
{

}

MapBlock::~MapBlock(){

}

MapSpirit *MapBlock::clone()
{
    MapBlock *b = new MapBlock(getId(),getName());
    for(auto s:spirits){
        b->addSpirit(s);
    }
    return b;
}
