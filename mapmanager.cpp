#include "mapmanager.h"
#include "sqlite3/CppSQLite3.h"
#include <iostream>

#include "Common.h"

MapManager* MapManager::p = new MapManager();

MapManager::MapManager():image_colors(NULL),mapModifying(false)
{
}

void MapManager::checkTable()
{
    //检查表
    CppSQLite3DB db;
    db.open(DB_File);
    if(!db.tableExists("agv_station")){
        db.execDML("create table agv_station(id int, x int,y int,name char(64));");
    }
    if(!db.tableExists("agv_line")){
        db.execDML("create table agv_line(id int, startStation int,endStation int,length int);");
    }
}

//载入地图
bool MapManager::load()
{
    checkTable();
    //载入数据
    mapModifying = true;
    if(!loadFromDb())
    {
        mapModifying = false;
        return false;
    }
//        if(!loadFromImg()){
//            mapModifying = false;
//            return false;
//        }
    mapModifying = false;
    return true;
}

//保存到数据库
bool MapManager::save()
{
    try{
        CppSQLite3DB db;
        db.open(DB_File);
        if(!db.tableExists("agv_station")){
            db.execDML("create table agv_station(id int, x int,y int,name char(64));");
        }else{
            db.execDML("delete from agv_station;");
        }
        if(!db.tableExists("agv_line")){
            db.execDML("create table agv_line(id int, startStation int,endStation int,length int);");
        }else{
            db.execDML("delete from agv_line;");
        }

        db.execDML("begin transaction;");
        char buf[256];
        for(auto station:m_stations){
            sprintf(buf, "insert into agv_station values (%d, %d,%d,%s);", station->id, station->x,station->y,station->name.c_str());
            db.execDML(buf);
        }
        for(auto line:m_lines){
            sprintf(buf, "insert into agv_line values (%d, %d,%d,%d);", line->id, line->startStation,line->endStation,line->length);
            db.execDML(buf);
        }
        db.execDML("commit transaction;");
    }catch(CppSQLite3Exception &e){
        std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
        return false;
    }catch(std::exception e){
        std::cerr << e.what()  << std::endl;
        return false;
    }
    return true;
}

//从数据库中载入地图
bool MapManager::loadFromDb()
{
    try{
        CppSQLite3DB db;
        std::cout << "SQLite Version: " << db.SQLiteVersion() << std::endl;
        db.open(DB_File);

        if(!db.tableExists("agv_station")){
            db.execDML("create table agv_station(id int, x int,y int,name char(64));");
        }else{
            db.execDML("delete from agv_station;");
        }
        if(!db.tableExists("agv_line")){
            db.execDML("create table agv_line(id int, startStation int,endStation int,length int);");
        }else{
            db.execDML("delete from agv_line;");
        }

        CppSQLite3Table table_station = db.getTable("select id,x,y,name from agv_station;");
        if(table_station.numFields()!=4)return false;
        for (int row = 0; row < table_station.numRows(); row++)
        {
            table_station.setRow(row);

            if(table_station.fieldIsNull(0) ||table_station.fieldIsNull(1) ||table_station.fieldIsNull(2))return false;
            AgvStation *station = new AgvStation;
            station->id = atoi(table_station.fieldValue(0));
            station->x = atoi(table_station.fieldValue(1));
            station->y = atoi(table_station.fieldValue(2));
            if(!table_station.fieldIsNull(3))
                station->name = std::string(table_station.fieldValue(0));
            m_stations.push_back(station);
        }

        CppSQLite3Table table_line = db.getTable("select id,startStation,endStation,length from agv_line;");
        if(table_line.numFields()!=4)return false;
        for (int row = 0; row < table_line.numRows(); row++)
        {
            table_line.setRow(row);

            if(table_line.fieldIsNull(0) ||table_line.fieldIsNull(1) ||table_line.fieldIsNull(2) ||table_line.fieldIsNull(3))return false;
            AgvLine *line = new AgvLine;
            line->id = atoi(table_line.fieldValue(0));
            int sId = atoi(table_line.fieldValue(1));
            int eId = atoi(table_line.fieldValue(2));
            line->length = atoi(table_line.fieldValue(3));
            for(auto station:m_stations){
                if(station->id == sId)line->startStation = station;
                if(station->id == eId)line->endStation = station;
            }
            if(line->startStation==NULL||line->endStation==NULL)return false;
            m_lines.push_back(line);
        }

        getReverseLines();
        getAdj();
    }catch(CppSQLite3Exception &e){
        std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
        return false;
    }catch(std::exception e){
        std::cerr << e.what()  << std::endl;
        return false;
    }
    return true;
}

bool  MapManager::IsThisColor(cv::Mat gridMap, int x,int y, cv::Scalar color)
{
    bool bflag = false;

    if(
            (gridMap.ptr<cv::Vec3b>(y)[x][0]==color[0]) &&
            (gridMap.ptr<cv::Vec3b>(y)[x][1]==color[1]) &&
            (gridMap.ptr<cv::Vec3b>(y)[x][2]==color[2])   )
        bflag = true;

    return  bflag;
}


bool MapManager::loadFromImg(std::string imgfile, int _gridsize)
{
    imageGridSize = _gridsize;
    try{
        //载入图像
        cv::Mat  gridmap = cv::imread(imgfile);
        //标记每个栅格的颜色
        getImgColors(gridmap);
        //获取交叉点，作为站点
        getImgStations();
        //对其他点作成线路
        getImgLines();
        //对所有线路标记反向线
        getReverseLines();
        //对所有线路可到达的其他线路
        getAdj();
        //保存到数据库
        save();
    }catch(std::exception e){
        std::cout<<e.what()<<std::endl;
        return false;
    }
}

//获取最优路径
std::vector<AgvLine *> MapManager::getBestPath(Agv *agv,AgvStation *lastStation, AgvStation *startStation, AgvStation* endStation, int &distance, bool changeDirect)
{
    distance = DISTANCE_INFINITY;
    if(mapModifying) return std::vector<AgvLine *>();
    int disA = DISTANCE_INFINITY;
    int disB = DISTANCE_INFINITY;

    std::vector<AgvLine *> a = getPath(agv, lastStation, startStation, endStation, disA, false);
    std::vector<AgvLine *> b;
    if (changeDirect) {
        b = getPath(agv, startStation, lastStation, endStation, disB, true);
        if (disA != DISTANCE_INFINITY && disB != DISTANCE_INFINITY) {
            distance = disA < disB ? disA : disB;
            if (disA < disB)return a;
            return b;
        }
    }
    if (disA != DISTANCE_INFINITY) {
        distance = disA;
        return a;
    }
    distance = disB;
    return b;
}

std::vector<AgvLine *> MapManager::getPath(Agv *agv, AgvStation *lastStation, AgvStation *startStation, AgvStation* endStation, int &distance, bool changeDirect)
{
    std::vector<AgvLine *> result;

    distance = DISTANCE_INFINITY;

    if (lastStation == nullptr) lastStation = startStation;

    if (lastStation == nullptr)return result;
    if (startStation == nullptr)return result;
    if (endStation == nullptr)return result;
    if(startStation->occuAgv>0 && startStation->occuAgv!=agv)return result;
    if(startStation->occuAgv>0 && endStation->occuAgv!=agv)return result;

    if (startStation == endStation) {
        if (changeDirect && lastStation != startStation) {
            for(auto line:m_lines){
                if(line->startStation == lastStation && line->endStation == startStation){
                    result.push_back(line);
                    distance = line->length;
                }
            }
        }
        else {
            distance = 0;
        }
        return result;
    }

    std::multimap<int, AgvLine *> Q;//key -->  distance ;value --> station;

    for(auto line:m_lines){
        line->father = NULL;
        line->distance = DISTANCE_INFINITY;
        line->color = AGV_LINE_COLOR_WHITE;
    }

    //增加一种通行的判定：
    //同事AGV2 要从 C点 到达 D点，同事路过B点。
    //如果AGV1 要从 A点 到达 B点。如果AGV1先到达B点，会导致AGV2 无法继续运行。
    //判定终点的线路 是否占用
    //endPoint是终点，lastPoint是到达endPoint的上一站。
    {
        for(auto templine:m_lines){
            if(templine->startStation == endStation){
                if (templine->occuAgvs.size() > 1 || (templine->occuAgvs.size() == 1 && (*(templine->occuAgvs.begin())) != agv)) {
                    //TODO:该方式到达这个地方 不可行.该线路 置黑、
                    AgvLine* llid = m_reverseLines[templine];
                    llid->color = AGV_LINE_COLOR_BLACK;
                    llid->distance = DISTANCE_INFINITY;
                }
            }
        }
    }


    if (lastStation == startStation) {
        for (auto l : m_lines) {
            if (l->startStation == lastStation) {
                AgvLine *reverse = m_reverseLines[l];
                if (reverse->occuAgvs.size() == 0 && (l->endStation->occuAgv == nullptr || l->endStation->occuAgv == agv)) {
                    if (l->color == AGV_LINE_COLOR_BLACK)continue;
                    l->distance = l->length;
                    l->color = AGV_LINE_COLOR_GRAY;
                    Q.insert(std::make_pair(l->distance, l));
                }
            }
        }
    }
    else {
        for (auto l : m_lines) {
            if (l->startStation == lastStation && l->endStation == lastStation) {
                AgvLine *reverse = m_reverseLines[l];
                if (reverse->occuAgvs.size() == 0 && (l->endStation->occuAgv == nullptr || l->endStation->occuAgv == agv)) {
                    if (l->color == AGV_LINE_COLOR_BLACK)continue;
                    l->distance = 0;
                    l->color = AGV_LINE_COLOR_GRAY;
                    Q.insert(std::make_pair(l->distance, l));
                    break;
                }
            }
        }
    }

    while (Q.size() > 0) {
        auto front = Q.begin();
        AgvLine *startLine = front->second;

        if (m_adj.find(startLine) == m_adj.end()) {
            startLine->color = AGV_LINE_COLOR_BLACK;
            for (auto ll = Q.begin();ll != Q.end();) {
                if (ll->second == startLine) {
                    ll = Q.erase(ll);
                }
                else {
                    ++ll;
                }
            }
            continue;
        }
        for (auto line : m_adj[startLine]) {
            if (line->color == AGV_LINE_COLOR_BLACK)continue;
            if (line->color == AGV_LINE_COLOR_WHITE) {
                AgvLine *reverse = m_reverseLines[line];
                if (reverse->occuAgvs.size() == 0 && (line->endStation->occuAgv == nullptr || line->endStation->occuAgv == agv)) {
                    line->distance = startLine->distance + line->length;
                    line->color = AGV_LINE_COLOR_GRAY;
                    line->father = startLine;
                    Q.insert(std::make_pair(line->distance, line));
                }
            }
            else if (line->color == AGV_LINE_COLOR_GRAY) {
                if (line->distance > startLine->distance + line->length) {
                    AgvLine *reverse = m_reverseLines[line];
                    if (reverse->occuAgvs.size() == 0 && (line->endStation->occuAgv == nullptr || line->endStation->occuAgv == agv)) {
                        int old_distance = line->distance;
                        line->distance = startLine->distance + line->length;
                        line->father = startLine;
                        //删除旧的
                        for (auto iiitr = Q.begin();iiitr != Q.end();) {
                            if (iiitr->second == line && iiitr->first == old_distance) {
                                iiitr = Q.erase(iiitr);
                            }
                        }
                        //加入新的
                        Q.insert(std::make_pair(line->distance, line));
                    }
                }
            }
        }

        startLine->color = AGV_LINE_COLOR_BLACK;
        for (auto ll = Q.begin();ll != Q.end();) {
            if (ll->second == startLine && ll->first == startLine->distance) {
                ll = Q.erase(ll);
            }
            else {
                ++ll;
            }
        }
    }

    AgvLine *index = NULL;
    int minDis = DISTANCE_INFINITY;

    for (auto ll : m_lines) {
        if (ll->endStation == endStation) {
            if (ll->distance < minDis) {
                minDis = ll->distance;
                index = ll;
            }
        }
    }

    distance = minDis;

    while (true) {
        if (index == NULL)break;
        result.push_back(index);
        index = index->father;
    }
    std::reverse(result.begin(), result.end());

    if (result.size() > 0 && lastStation != startStation) {
        if (!changeDirect) {
            AgvLine * agv_line = *(result.begin());
            if (agv_line->startStation == lastStation && agv_line->endStation == startStation) {
                result.erase(result.begin());
            }
        }
    }

    return result;
}



//对图像中每个栅格的颜色做标记
void MapManager::getImgColors(cv::Mat &gridmap) throw (std::exception)
{
    if(gridmap.cols<=0||gridmap.rows<0){
        throw std::runtime_error(std::string("空图像"));
    }
    imageGridColumn    = gridmap.cols / imageGridSize;
    imageGridRow    = gridmap.rows / imageGridSize;
    imageGridHalfSize = (imageGridSize/2);
    if(imageGridSize%2!=0)++imageGridHalfSize;

    image_colors = new ImageColors(imageGridColumn,imageGridRow);
    for(int i=0;i<imageGridRow;++i) {
        for(int j = 0;j<imageGridColumn;++j){
            int posY = i*imageGridSize+imageGridHalfSize;
            int posX = j*imageGridSize+imageGridHalfSize;
            for(int k = 1;k<MAP_GRID_COLOR_LENGTH;++k){
                if(IsThisColor(gridmap,posX,posY,color_table[k].scalar)){
                    image_colors->setColor(i,j,color_table[k].colorId);
                    break;
                }
            }
        }
    }
}


void MapManager::getImgStations()
{
    for(int i=0;i<imageGridRow;++i) {
        for(int j = 0;j<imageGridColumn;++j){
            if(image_colors->getColor(i,j) != MAP_GRID_COLOR_BLUE)continue;

            int lineAmount = 0;
            bool first = false;
            bool last = false;
            bool temp;
            for(int k=0;k<sizeof(around_pos)/sizeof(around_pos[0]);++k)
            {
                int row = i+around_pos[k].row;
                int column = j+around_pos[k].column;
                if(row<0||column<0||row>=imageGridRow||column>imageGridColumn)
                {
                    temp = false;
                }else{
                    temp = image_colors->getColor(row,column) == MAP_GRID_COLOR_BLUE;
                }
                if(k == 0){
                    if(temp)lineAmount++;
                    first = temp;
                }else{
                    if(!last && temp){
                        lineAmount++;
                        if(k+1 == sizeof(around_pos)/sizeof(around_pos[0])){
                            if(first)lineAmount--;
                        }
                    }
                }
                last = temp;
            }

            if(lineAmount >2){
                //这个是交叉点
                AgvStation *station = new AgvStation;
                station->id = m_stations.size();
                station->x = j;
                station->y = i;
                m_stations.push_back(station);
            }else if(lineAmount == 1){
                //这个是端点
                AgvStation *station = new AgvStation;
                station->id = m_stations.size()+1;//ID>0
                station->x = j;
                station->y = i;
                m_stations.push_back(station);
            }
        }
    }

    for(auto station:m_stations){
        std::cout<<"x="<<station->x<<" y="<<station->y<<" id="<<station->id<<std::endl;
    }
}

//返回下一个站点
AgvStation* MapManager::recrsion(int row,int column,int stationId,int ***pps,int &l_length,int s_length){
    for(int k=0;k<sizeof(around_pos)/sizeof(around_pos[0]);++k)
    {
        int rrow = row+around_pos[k].row;
        int ccolumn = column+around_pos[k].column;
        if(rrow<0||ccolumn<0||rrow>=imageGridRow||ccolumn>imageGridColumn)continue;
        if(image_colors->getColor(row,column) == MAP_GRID_COLOR_BLUE){
            if((*pps)[rrow][ccolumn]!=stationId){

                if(k%2 == 0)s_length+=1;
                else l_length +=1;
                for(auto station:m_stations){
                    if(station->x == ccolumn && station->y == rrow)return station;
                }
                (*pps)[rrow][ccolumn] = stationId;
                return recrsion(rrow,ccolumn,stationId,pps,l_length,s_length);
            }
        }
    }

    return nullptr;
}

void MapManager::getImgLines()
{
    int **ppps =new int*[imageGridRow];
    for(auto i=0;i<imageGridRow;i++)
        ppps[i]=new int[imageGridColumn];



    for(AgvStation *station:m_stations){
        //对于一个站点
        for(int i=0;i<imageGridRow;++i){
            for(int j=0;j<imageGridColumn;++j){
                ppps[i][j] = 0;
            }
        }
        //自身点位的站点ID
        ppps[station->y][station->x] = station->id;
        //周围相邻点位的站点ID
        for(int k=0;k<sizeof(around_pos)/sizeof(around_pos[0]);++k)
        {
            int row = station->y +around_pos[k].row;
            int column = station->x+around_pos[k].column;
            if(row<0||column<0||row>=imageGridRow||column>imageGridColumn)continue;
            if(image_colors->getColor(row,column) == MAP_GRID_COLOR_BLUE){
                ppps[row][column] = station->id;
            }
        }
        //周围相邻点位的站点ID
        for(int k=0;k<sizeof(around_pos)/sizeof(around_pos[0]);++k)
        {
            int row = station->y +around_pos[k].row;
            int column = station->x+around_pos[k].column;
            if(row<0||column<0||row>=imageGridRow||column>imageGridColumn)continue;
            if(image_colors->getColor(row,column) == MAP_GRID_COLOR_BLUE){
                int l_length = 0;
                int s_length = 0;
                AgvStation* nextStationId = recrsion(row,column,station->id,&ppps,l_length,s_length);
                if(nextStationId!=0){
                    AgvLine *line = new AgvLine;
                    line->id = m_lines.size()+1;
                    line->startStation = station;
                    line->endStation = nextStationId;
                    line->length = l_length+sqrt(2)*s_length;

                    m_lines.push_back(line);
                }
            }
        }
    }
}

void MapManager::getReverseLines()
{
    for(auto a:m_lines){
        for(auto b:m_lines){
            if(a->id == b->id)continue;
            if(a->startStation == b->endStation && a->endStation == b->startStation){
                m_reverseLines[a] = b;
            }
        }
    }
}

void MapManager::getAdj()
{
    for(auto a:m_lines){
        for(auto b:m_lines){
            if(a == b)continue;
            if(a->endStation == b->startStation && a->endStation != b->startStation){
                if(m_adj.find(a)!=m_adj.end()){
                    m_adj[a].push_back(b);
                }else{
                    std::vector<AgvLine *> ll;
                    ll.push_back(b);
                    m_adj.insert(std::make_pair(a,ll));
                }
            }
        }
    }
}
