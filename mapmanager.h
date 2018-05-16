#ifndef MAPMANAGER_H
#define MAPMANAGER_H
#include <map>
#include <utility>
#include <algorithm>
#include <exception>
#include <atomic>
#include <vector>
#include "agvstation.h"
#include "agvline.h"
#include "AgvBlock.h"
#include "agv.h"
#include "common.h"
#include "utils/noncopyable.h"

#include "network/tcpsession.h"


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

    std::vector<AgvStationPtr> getStations(){return  m_stations;}

    //用户接口
    void interSetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interGetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);

    void interTrafficControlStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficControlLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficReleaseStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficReleaseLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
protected:
    MapManager();
private:
    void clear();
    AgvLinePtr  getLineId(AgvStationPtr startStation, AgvStationPtr endStation);
    std::vector<AgvStationPtr> m_stations;//站点
    std::vector<AgvLinePtr> m_lines;//线路
	std::vector<AgvBlockPtr> m_blocks;//区块
    std::map<AgvLinePtr, std::vector<AgvLinePtr> > m_adj;  //从一条线路到另一条线路的关联表
    std::map<AgvLinePtr, AgvLinePtr> m_reverseLines;//线路和它的反方向线路的集合。

    std::vector<AgvLinePtr> getPath(AgvPtr agv, AgvStationPtr lastStation, AgvStationPtr startStation, AgvStationPtr endStation, int &distance, bool changeDirect);
    void checkTable();

    void getReverseLines();
    void getAdj();

    std::atomic_bool mapModifying;
};

#endif // MAPMANAGER_H
