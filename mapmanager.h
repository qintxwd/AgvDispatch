#ifndef MAPMANAGER_H
#define MAPMANAGER_H
#include <map>
#include <utility>
#include <algorithm>
#include <exception>
#include <atomic>
#include <vector>
#include <opencv2/opencv.hpp>
#include "agvstation.h"
#include "agvline.h"
#include "agv.h"
#include "Common.h"

//自定义颜色标记
typedef enum{
    MAP_GRID_COLOR_UNKNOWN = 0,//起始位置

    MAP_GRID_COLOR_WHITE,
    MAP_GRID_COLOR_BLACK,
    MAP_GRID_COLOR_BLUE,
    MAP_GRID_COLOR_GREEN,
    MAP_GRID_COLOR_RED,
    MAP_GRID_COLOR_GRAY,

    MAP_GRID_COLOR_LENGTH,//结束位置
}ENUM_MAP_GRID_COLOR;

const static struct
{
    ENUM_MAP_GRID_COLOR colorId;
    cv::Scalar scalar;
} color_table[] =
{
    {MAP_GRID_COLOR_UNKNOWN,cv::Scalar(0,0,0)},
    {MAP_GRID_COLOR_WHITE,cv::Scalar(255,255,255)},
    {MAP_GRID_COLOR_BLACK,cv::Scalar(0,0,0)},
    {MAP_GRID_COLOR_BLUE,cv::Scalar(255,0,0)},
    {MAP_GRID_COLOR_GREEN,cv::Scalar(0,255,0)},
    {MAP_GRID_COLOR_RED,cv::Scalar(0,0,255)},
    {MAP_GRID_COLOR_GRAY,cv::Scalar(0,0,0)},
};

const static struct{
    int row;
    int column;
} around_pos []=
{
    { -1, -1},{ 0, -1 },{ 1, -1 },{ 1, 0 },
    { 1, 1 },{ 0, 1 },{ -1, 1},{ -1, 0 },
};


//位置和颜色
class ImageColors{
public:
    ImageColors(int _column,int _row){
        column = _column;
        row = _row;

        colors=new int*[row];       //分配一个指针数组，将其首地址保存在b中
        for(auto i=0;i<row;i++)             //为指针数组的每个元素分配一个数组
            colors[i]=new int[column];

        for(int i=0;i<row;++i)
            for(int j=0;j<column;++j)
                colors[i][j] = 0;
    }

    int getColor(int row,int column){
        return colors[row][column];
    }

    void setColor(int row,int column,int color){
        colors[row][column] = color;
    }

private:
    int **colors;
    int column;
    int row;
};


//地图 由两部分助成： 点AgvStation 和 点之间的连线AgvLine
class MapManager
{
public:

    typedef struct _line_points{
        int column;
        int row;
        int stationId;//标记它挂靠的站点
        bool operator == (const  _line_points &b){
            return row == b.row && column == b.column;
        }
    }LINE_POINTS;

    static MapManager* getInstance(){
        return p;
    }

    //载入地图
    bool load();
    //保存地图
    bool save();
    //从配置或者数据库中载入地图
    bool loadFromDb();
    //从栅格图片中导入地图
    bool loadFromImg(std::string imgfile = "grid.bmp",int imageGridSize = 5);

    //一个Agv占领一个站点
    void occuStation(AgvStation *station, Agv *occuAgv);

    //线路的反向占用//这辆车行驶方向和线路方向相反
    void addOccuLine(AgvLine* line, Agv* occuAgv);

    //如果车辆占领该站点，释放
    void freeStation(AgvStation* station, Agv* occuAgv);

    //如果车辆在线路的占领表中，释放出去
    void freeLine(AgvLine *line, Agv* occuAgv);

    //获取最优路径
    std::vector<AgvLine *> getBestPath(Agv *agv,AgvStation *lastStation, AgvStation *startStation, AgvStation *endStation, int &distance, bool changeDirect = CAN_CHANGE_DIRECTION);

    //获取反向线路
    AgvLine* getReverseLine(AgvLine *line);

protected:
    MapManager();
private:
    static MapManager* p;
private:
    AgvLine* getLineId(AgvStation *startStation, AgvStation *endStation);
    std::vector<AgvStation *> m_stations;//站点
    std::vector<AgvLine *> m_lines;//线路
    std::map<AgvLine *, std::vector<AgvLine *> > m_adj;  //从一条线路到另一条线路的关联表
    std::map<AgvLine *, AgvLine *> m_reverseLines;//线路和它的反方向线路的集合。

    std::vector<AgvLine *> getPath(Agv *agv, AgvStation *lastStation, AgvStation *startStation, AgvStation *endStation, int &distance, bool changeDirect);
    void checkTable();
    //以下是从image 读取 路径和站点
    void getImgColors(cv::Mat &gridmap) throw (std::exception);
    void getImgStations();
    void getImgLines();
    void getReverseLines();
    void getAdj();
    bool IsThisColor(cv::Mat  gridMap, int x, int y, cv::Scalar color);

    int imageGridSize;//栅格尺寸
    int imageGridHalfSize;
    int imageGridColumn;//栅格列数
    int imageGridRow;//栅格行数
    ImageColors *image_colors;//栅格的颜色数组

    AgvStation* recrsion(int row,int column,int stationId,int ***pps,int &l_length,int s_length);

    std::atomic_bool mapModifying;
};

#endif // MAPMANAGER_H
