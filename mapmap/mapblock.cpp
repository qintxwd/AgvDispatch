#include "mapblock.h"

MapBlock::MapBlock(int _id, std::string _name) :
    MapSpirit(_id,_name,Map_Sprite_Type_Block)
{

}

MapBlock::MapBlock(const MapBlock& b):
    MapSpirit(b),
    spirits(b.spirits)
{
}
