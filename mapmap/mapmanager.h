#ifndef MAPMANAGER_H
#define MAPMANAGER_H
#include <map>
#include <utility>
#include <algorithm>
#include <exception>
#include <atomic>
#include <vector>
#include "onemap.h"
#include "../agv.h"
#include "../common.h"
#include "../utils/noncopyable.h"
#include "../network/tcpsession.h"


class MapManager;
using MapManagerPtr = std::shared_ptr<MapManager>;

//地图 由两部分助成： 点AgvStation 和 点之间的连线AgvLine
class MapManager : public noncopyable, public std::enable_shared_from_this<MapManager>
{
public:

    static MapManagerPtr getInstance(){
        static MapManagerPtr m_inst = MapManagerPtr(new MapManager());
        return m_inst;
    }

    //载入地图
    bool load();
    //保存地图
    bool save();

	MapSpirit *getMapSpiritById(int id);

    MapSpirit *getMapSpiritByName(std::string name);

    MapPath *getMapPathByStartEnd(int start,int end);

    //一个Agv占领一个站点
    void occuStation(int station, AgvPtr occuAgv);

    //线路的反向占用//这辆车行驶方向和线路方向相反
    void addOccuLine(int line, AgvPtr occuAgv);

    //如果车辆占领该站点，释放
    void freeStation(int station, AgvPtr occuAgv);

    //如果车辆在线路的占领表中，释放出去
    void freeLine(int line, AgvPtr occuAgv);

    //获取最优路径
    std::vector<int> getBestPath(int agv,int lastStation, int startStation, int endStation, int &distance, bool changeDirect = CAN_CHANGE_DIRECTION);

    //用户接口
    void interSetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interGetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);

    void interTrafficControlStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficControlLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficReleaseStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
    void interTrafficReleaseLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request);
private:
    MapManager();

    //从配置或者数据库中载入地图
    bool loadFromDb();
    //清空所有地图信息
    void clear();

    OneMap g_onemap;//地图节点

    std::map<int,std::vector<int> > m_adj;// lineA -- lines{ from line A can reach which lines}
    std::map<int,int> m_reverseLines;

    std::map<int,std::vector<int> > line_occuagvs;//一条线路 及其上面的agv
    std::map<int,int> station_occuagv;//一个站点，及当前占用改站点的agv
    std::map<int,int> block_occuagv;//一个区块，及当前区块占用agv

    std::vector<int> getPath(int agv, int lastStation, int startStation, int endStation, int &distance, bool changeDirect);
    void checkTable();

    void getReverseLines();
    void getAdj();

	bool pathPassable(MapPath *line, int agvId);

    std::atomic_bool mapModifying;
};

#endif // MAPMANAGER_H
