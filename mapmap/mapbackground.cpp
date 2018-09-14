#include "mapbackground.h"
#include <string.h>
MapBackground::MapBackground(int _id, std::string _name,  char *_img_data, int _img_data_len, int _width, int _height, std::string fileName):
    MapSpirit(_id,_name,Map_Sprite_Type_Background),
    x(0),
    y(0),
    width(_width),
    height(_height),
    img_data(nullptr),
    img_data_len(_img_data_len),
    imgFileName(fileName)
{
    img_data = new char[img_data_len];
    memcpy(img_data,_img_data,img_data_len);
}

MapBackground::~MapBackground()
{
    if(img_data!=nullptr){
        delete img_data;
        img_data = nullptr;
    }
}

MapSpirit* MapBackground::clone()
{
    char *copy = new char[img_data_len];
    memcpy(copy,img_data,img_data_len);
    MapBackground *b = new MapBackground(getId(),getName(),copy,getImgDataLen(),getWidth(),getHeight(),getFileName());
    b->setX(x);
    b->setY(y);
    return b;
}

