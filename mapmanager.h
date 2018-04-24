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
#include "common.h"
#include "utils/noncopyable.h"

#include "network/tcpsession.h"

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

class MapManager;
using MapManagerPtr = std::shared_ptr<MapManager>;

//地图 由两部分助成： 点AgvStation 和 点之间的连线AgvLine
class MapManager : public noncopyable, public std::enable_shared_from_this<MapManager>
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

    static MapManagerPtr getInstance(){
        static MapManagerPtr m_inst = MapManagerPtr(new MapManager());
        return m_inst;
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
    void occuStation(AgvStationPtr station, AgvPtr occuAgv);

    //线路的反向占用//这辆车行驶方向和线路方向相反
    void addOccuLine(AgvLinePtr line, AgvPtr occuAgv);

    //如果车辆占领该站点，释放
    void freeStation(AgvStationPtr station, AgvPtr occuAgv);

    //如果车辆在线路的占领表中，释放出去
    void freeLine(AgvLinePtr line, AgvPtr occuAgv);

    //获取最优路径
    std::vector<AgvLinePtr> getBestPath(AgvPtr agv,AgvStationPtr lastStation, AgvStationPtr startStation, AgvStationPtr endStation, int &distance, bool changeDirect = CAN_CHANGE_DIRECTION);

    //获取反向线路
    AgvLinePtr getReverseLine(AgvLinePtr line);

    AgvStationPtr getStationById(int id);

	AgvLinePtr getLineById(int id);

    //用户接口
    void interCreateStart(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interCreateAddStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interCreateAddLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interCreateAddArc(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interCreateFinish(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interListStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
    void interListLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);

	void interTrafficControlStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
	void interTrafficControlLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
	void interTrafficReleaseStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
	void interTrafficReleaseLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg);
protected:
    MapManager();
private:
    void clear();
    AgvLinePtr  getLineId(AgvStationPtr startStation, AgvStationPtr endStation);
    std::vector<AgvStationPtr> m_stations;//站点
    std::vector<AgvLinePtr> m_lines;//线路
    std::map<AgvLinePtr, std::vector<AgvLinePtr> > m_adj;  //从一条线路到另一条线路的关联表
    std::map<AgvLinePtr, AgvLinePtr> m_reverseLines;//线路和它的反方向线路的集合。

    std::vector<AgvLinePtr> getPath(AgvPtr agv, AgvStationPtr lastStation, AgvStationPtr startStation, AgvStationPtr endStation, int &distance, bool changeDirect);
    void checkTable();
    //以下是从image 读取 路径和站点
    void getImgColors(cv::Mat &gridmap);
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

    AgvStationPtr recrsion(int row,int column,int stationId,int ***pps,int &l_length,int s_length);

    std::atomic_bool mapModifying;
};

#endif // MAPMANAGER_H
