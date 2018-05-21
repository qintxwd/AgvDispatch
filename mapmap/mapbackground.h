#ifndef MAPBACKGROUND_H
#define MAPBACKGROUND_H

#include "mapspirit.h"

class MapBackground : public MapSpirit
{
public:
    MapBackground(int _id, std::string _name, char *_img_data, int _img_data_len, int _width, int _height, std::string fileName);

    virtual ~MapBackground();
    virtual MapSpirit *clone() ;
    MapBackground(const MapBackground& b) = delete;

    void setX(int _x){x=_x;}
    void setY(int _y){y=_y;}
    void setWidth(int _width){width=_width;}
    void setHeight(int _height){height=_height;}

    int getX(){return x;}
    int getY(){return y;}
    int getWidth(){return width;}
    int getHeight(){return height;}

    std::string getFileName(){return imgFileName;}
    char* getImgData(){return img_data;}
    int getImgDataLen(){return img_data_len;}

    void setFileName(std::string _filename){imgFileName = _filename;}
    void setImgData(char *_img_data){img_data=_img_data;}
    void setImgDataLen(int _img_data_len){img_data_len=_img_data_len;}
private:
    char *img_data;//图片数据
    int img_data_len;//图片数据长度
    std::string imgFileName;
    int x;
    int y;
    int width;
    int height;
};

#endif // MAPBACKGROUND_H
