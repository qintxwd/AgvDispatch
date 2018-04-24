#include "mapmanager.h"
#include "sqlite3/CppSQLite3.h"
#include "msgprocess.h"
#include "common.h"
#include "taskmanager.h"
#include "userlogmanager.h"

MapManager::MapManager():image_colors(NULL),mapModifying(false)
{
}

void MapManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_station")){
            g_db.execDML("create table agv_station(id INTEGER, x INTEGER,y INTEGER,name char(64));");
        }
        if(!g_db.tableExists("agv_line")){
            g_db.execDML("create table agv_line(id INTEGER, startStation INTEGER,endStation INTEGER,length INTEGER);");
        }
    }catch(CppSQLite3Exception &e){
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
        return ;
    }catch(std::exception e){
        LOG(ERROR) << e.what();
        return ;
    }
}

AgvLinePtr MapManager::getReverseLine(AgvLinePtr line)
{
    if(m_reverseLines.find(line)==m_reverseLines.end())return nullptr;
    return m_reverseLines[line];
}

AgvLinePtr MapManager::getLineById(int id)
{
	for (auto l : m_lines) {
		if (l->id == id)return l;
	}
	return nullptr;
}

AgvStationPtr MapManager::getStationById(int id)
{
    for(auto s:m_stations){
        if(s->id == id)return s;
    }
    return nullptr;
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
        if(!g_db.tableExists("agv_station")){
            g_db.execDML("create table agv_station(id INTEGER, x INTEGER,y INTEGER,name char(64));");
        }else{
            g_db.execDML("delete from agv_station;");
        }
        if(!g_db.tableExists("agv_line")){
            g_db.execDML("create table agv_line(id INTEGER, startStation INTEGER,endStation INTEGER,length INTEGER);");
        }else{
            g_db.execDML("delete from agv_line;");
        }

        g_db.execDML("begin transaction;");
        char buf[SQL_MAX_LENGTH];
        for(auto station:m_stations){
            sprintf_s(buf, SQL_MAX_LENGTH, "insert into agv_station values (%d, %d,%d,%s);", station->id, station->x,station->y,station->name.c_str());
            g_db.execDML(buf);
        }
        for(auto line:m_lines){
            sprintf_s(buf, SQL_MAX_LENGTH, "insert into agv_line values (%d,%d,%d,%d);", line->id, line->startStation->id,line->endStation->id,line->length);
            g_db.execDML(buf);
        }
        g_db.execDML("commit transaction;");
    }catch(CppSQLite3Exception &e){
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
        return false;
    }catch(std::exception e){
        LOG(ERROR) << e.what();
        return false;
    }
    return true;
}

//从数据库中载入地图
bool MapManager::loadFromDb()
{
    try{
        if(!g_db.tableExists("agv_station")){
            g_db.execDML("create table agv_station(id INTEGER, x INTEGER,y INTEGER,name char(64));");
        }
        if(!g_db.tableExists("agv_line")){
            g_db.execDML("create table agv_line(id INTEGER, startStation INTEGER,endStation INTEGER,length INTEGER);");
        }

        CppSQLite3Table table_station = g_db.getTable("select id,x,y,name from agv_station;");
        if(table_station.numRows()>0 && table_station.numFields()!=4)return false;
        for (int row = 0; row < table_station.numRows(); row++)
        {
            table_station.setRow(row);

            if(table_station.fieldIsNull(0) ||table_station.fieldIsNull(1) ||table_station.fieldIsNull(2))return false;
            AgvStationPtr station = AgvStationPtr(new AgvStation);
            station->id = atoi(table_station.fieldValue(0));
            station->x = atoi(table_station.fieldValue(1));
            station->y = atoi(table_station.fieldValue(2));
            if(!table_station.fieldIsNull(3))
                station->name = std::string(table_station.fieldValue(0));
            m_stations.push_back(station);
        }

        CppSQLite3Table table_line = g_db.getTable("select id,startStation,endStation,length from agv_line;");
        if(table_line.numRows()>0 && table_line.numFields()!=4)return false;
        for (int row = 0; row < table_line.numRows(); row++)
        {
            table_line.setRow(row);

            if(table_line.fieldIsNull(0) ||table_line.fieldIsNull(1) ||table_line.fieldIsNull(2) ||table_line.fieldIsNull(3))return false;
            AgvLinePtr line = AgvLinePtr(new AgvLine);
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
        LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
        return false;
    }catch(std::exception e){
        LOG(ERROR) << e.what();
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
        LOG(ERROR)<<e.what();
        return false;
    }
    return true;
}

//获取最优路径
std::vector<AgvLinePtr> MapManager::getBestPath(AgvPtr agv,AgvStationPtr lastStation, AgvStationPtr startStation, AgvStationPtr endStation, int &distance, bool changeDirect)
{
    distance = DISTANCE_INFINITY;
    if(mapModifying) return std::vector<AgvLinePtr>();
    int disA = DISTANCE_INFINITY;
    int disB = DISTANCE_INFINITY;

    std::vector<AgvLinePtr> a = getPath(agv, lastStation, startStation, endStation, disA, false);
    std::vector<AgvLinePtr> b;
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

std::vector<AgvLinePtr> MapManager::getPath(AgvPtr agv, AgvStationPtr lastStation, AgvStationPtr startStation, AgvStationPtr endStation, int &distance, bool changeDirect)
{
    std::vector<AgvLinePtr> result;

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

    std::multimap<int, AgvLinePtr> Q;//key -->  distance ;value --> station;

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
                    AgvLinePtr  llid = m_reverseLines[templine];
                    llid->color = AGV_LINE_COLOR_BLACK;
                    llid->distance = DISTANCE_INFINITY;
                }
            }
        }
    }


    if (lastStation == startStation) {
        for (auto l : m_lines) {
            if (l->startStation == lastStation) {
                AgvLinePtr reverse = m_reverseLines[l];
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
                AgvLinePtr reverse = m_reverseLines[l];
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
        AgvLinePtr startLine = front->second;

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
                AgvLinePtr reverse = m_reverseLines[line];
                if (reverse->occuAgvs.size() == 0 && (line->endStation->occuAgv == nullptr || line->endStation->occuAgv == agv)) {
                    line->distance = startLine->distance + line->length;
                    line->color = AGV_LINE_COLOR_GRAY;
                    line->father = startLine;
                    Q.insert(std::make_pair(line->distance, line));
                }
            }
            else if (line->color == AGV_LINE_COLOR_GRAY) {
                if (line->distance > startLine->distance + line->length) {
                    AgvLinePtr reverse = m_reverseLines[line];
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

    AgvLinePtr index = NULL;
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
            AgvLinePtr  agv_line = *(result.begin());
            if (agv_line->startStation == lastStation && agv_line->endStation == startStation) {
                result.erase(result.begin());
            }
        }
    }

    return result;
}



//对图像中每个栅格的颜色做标记
void MapManager::getImgColors(cv::Mat &gridmap)
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
                AgvStationPtr station =AgvStationPtr(new AgvStation);
                station->id = m_stations.size();
                station->x = j;
                station->y = i;
                m_stations.push_back(station);
            }else if(lineAmount == 1){
                //这个是端点
                AgvStationPtr station = AgvStationPtr(new AgvStation);
                station->id = m_stations.size()+1;//ID>0
                station->x = j;
                station->y = i;
                m_stations.push_back(station);
            }
        }
    }

    for(auto station:m_stations){
        LOG(INFO)<<"x="<<station->x<<" y="<<station->y<<" id="<<station->id;
    }
}

//返回下一个站点
AgvStationPtr MapManager::recrsion(int row,int column,int stationId,int ***pps,int &l_length,int s_length){
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



    for(auto station:m_stations){
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
                AgvStationPtr nextStationId = recrsion(row,column,station->id,&ppps,l_length,s_length);
                if(nextStationId!=0){
                    AgvLinePtr line = AgvLinePtr(new AgvLine);
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
                    std::vector<AgvLinePtr > ll;
                    ll.push_back(b);
                    m_adj.insert(std::make_pair(a,ll));
                }
            }
        }
    }
}

void MapManager::clear()
{
    m_reverseLines.clear();
    m_adj.clear();
    m_stations.clear();
    m_lines.clear();
}

void MapManager::interCreateStart(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (TaskManager::getInstance()->hasTaskDoing())
    {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_TASKING;
        sprintf_s(response.return_head.error_info, "%s","there are some task is taking");
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+"重新设置地图");

        mapModifying = true;
        clear();
        try{
            g_db.execDML("delete from agv_station;");
            g_db.execDML("delete from agv_line;");
            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }

    conn->send(response);
}

void MapManager::interCreateAddStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (!mapModifying) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
    }
    else {
        if (msg.head.body_length != sizeof(STATION_INFO)) {
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error STATION_INFO length");
        }
        else {
            STATION_INFO station;
            memcpy_s(&station, sizeof(STATION_INFO),msg.body, sizeof(STATION_INFO));
            char buf[MSG_LONG_LONG_STRING_LEN];
            sprintf_s(buf,MSG_LONG_LONG_STRING_LEN, "insert into agv_station values (%d, %d,%d,%s);", station.id, station.x,station.y,station.name);
            try{
                g_db.execDML(buf);
                response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
            }catch(CppSQLite3Exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
                LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
            }catch(std::exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
                LOG(ERROR)<<"sqlerr code:"<<e.what();
            }
        }
    }
    conn->send(response);
}

void MapManager::interCreateAddLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (!mapModifying) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
    }
    else {
        if (msg.head.body_length != sizeof(AGV_LINE)) {
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error AGV_LINE length");
        }
        else {
            AGV_LINE line;
            memcpy_s(&line,sizeof(AGV_LINE), msg.body, sizeof(AGV_LINE));
            char buf[SQL_MAX_LENGTH];
            sprintf_s(buf,SQL_MAX_LENGTH, "INSERT INTO agv_line (id,line_startStation,line_endStation,line_length) VALUES (%d,%d,%d,%d);", line.id, line.startStation,line.endStation,line.length);
            try{
                g_db.execDML(buf);
                response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
            }catch(CppSQLite3Exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
                LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
            }catch(std::exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
                LOG(ERROR)<<"sqlerr code:"<<e.what();
            }

            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        }

    }
    conn->send(response);
}

void MapManager::interCreateFinish(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (!mapModifying) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+"重新设置地图完成");
        if(!save()){
            response.return_head.result = RETURN_MSG_RESULT_FAIL;
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_SAVE_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","save data to sql fail");
        }else{
            getReverseLines();
            getAdj();
            mapModifying = false;
            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        }
    }
    conn->send(response);

    //通知所有客户端，map更新了
    MsgProcess::getInstance()->notifyAll(ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE);
}

void MapManager::interListStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (mapModifying) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+"获取地图站点信息");
        response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        for (auto s : m_stations) {
            STATION_INFO info;
            info.id = s->id;
            info.x = s->x;
            info.y = s->y;
            info.floorId = s->floorId;
            info.occuagv = s->occuAgv->getId();
            sprintf_s(info.name,MSG_STRING_LEN,s->name.c_str(),s->name.length());
            memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(STATION_INFO));
            response.head.flag = 1;
            response.head.body_length = sizeof(STATION_INFO);
            conn->send(response);
        }
        response.head.body_length = 0;
        response.head.flag = 0;
    }
    conn->send(response);
}

void MapManager::interListLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;

    if (mapModifying) {
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is creating map");
    }
    else {
        UserLogManager::getInstance()->push(conn->getUserName()+"获取地图线路信息");
        response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
        for (auto l : m_lines) {
            AGV_LINE line;
            line.id = l->id;
            line.startStation = l->startStation->id;
            line.endStation = l->endStation->id;
            line.length = l->length;

            memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&line,sizeof(AGV_LINE));

            response.head.flag = 1;
            response.head.body_length = sizeof(AGV_LINE);
            conn->send(response);
        }
        response.head.flag = 0;
        response.head.body_length = 0;
    }
    conn->send(response);
}

void MapManager::interTrafficControlStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
	MSG_Response response;
	memset(&response, 0, sizeof(MSG_Response));
	memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
	response.head.body_length = 0;
	response.return_head.result = RETURN_MSG_RESULT_FAIL;

	if (mapModifying) {
		response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
		sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "is creating map");
	}
	else {
		if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
			sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error station id length");
		}
		else {
			int stationId = 0;
			memcpy_s(&stationId, sizeof(int32_t), msg.body, sizeof(int32_t));
			AgvStationPtr station = getStationById(stationId);
			if (station == nullptr) {
				response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
				sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded station id");
			}
			else {
				//存库...
				//TODO

				//设置
				AgvPtr occagv = AgvPtr(new Agv(TRAFFIC_OCCUR_AGV_ID, "", "", 0));
				station->occuAgv = occagv;

				response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			}			
		}
	}
	conn->send(response);
}

void MapManager::interTrafficReleaseLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
	MSG_Response response;
	memset(&response, 0, sizeof(MSG_Response));
	memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
	response.head.body_length = 0;
	response.return_head.result = RETURN_MSG_RESULT_FAIL;

	if (mapModifying) {
		response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
		sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "is creating map");
	}
	else {
		if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
			sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error line id length");
		}
		else {
			int lineId = 0;
			memcpy_s(&lineId, sizeof(int32_t), msg.body, sizeof(int32_t));
			AgvLinePtr line = getLineById(lineId);
			if (line == nullptr) {
				response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
				sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded line id");
			}
			else {
				//存库...
				//TODO
				//设置
				//是否已经被管制了
				bool already = false;
				for (auto agv : line->occuAgvs) {
					if (agv->getId() == TRAFFIC_OCCUR_AGV_ID) {
						already = true;
						break;
					}
				}

				if (!already) {
					AgvPtr occagv = AgvPtr(new Agv(TRAFFIC_OCCUR_AGV_ID, "", "", 0));
					line->occuAgvs.push_back(occagv);
				}

				response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			}
		}
	}
	conn->send(response);
}


void MapManager::interTrafficReleaseStation(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
	MSG_Response response;
	memset(&response, 0, sizeof(MSG_Response));
	memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
	response.head.body_length = 0;
	response.return_head.result = RETURN_MSG_RESULT_FAIL;

	if (mapModifying) {
		response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
		sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "is creating map");
	}
	else {
		if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
			sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error station id length");
		}
		else {
			int stationId = 0;
			memcpy_s(&stationId, sizeof(int32_t), msg.body, sizeof(int32_t));
			AgvStationPtr station = getStationById(stationId);
			if (station == nullptr) {
				response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
				sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded station id");
			}
			else {
				//存库...
				//TODO

				//设置
				if (station->occuAgv != nullptr && station->occuAgv->getId() == TRAFFIC_OCCUR_AGV_ID) {
					station->occuAgv = nullptr;
				}
				response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			}
		}
	}
	conn->send(response);
}

void MapManager::interTrafficControlLine(qyhnetwork::TcpSessionPtr conn, MSG_Request msg)
{
	MSG_Response response;
	memset(&response, 0, sizeof(MSG_Response));
	memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
	response.head.body_length = 0;
	response.return_head.result = RETURN_MSG_RESULT_FAIL;

	if (mapModifying) {
		response.return_head.error_code = RETURN_MSG_ERROR_CODE_CTREATING;
		sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "is creating map");
	}
	else {
		if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
			sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error line id length");
		}
		else {
			int lineId = 0;
			memcpy_s(&lineId, sizeof(int32_t), msg.body, sizeof(int32_t));
			AgvLinePtr line = getLineById(lineId);
			if (line == nullptr) {
				response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
				sprintf_s(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded line id");
			}
			else {
				for (auto itr = line->occuAgvs.begin(); itr != line->occuAgvs.end(); ) {
					if ((*itr)->getId() == TRAFFIC_OCCUR_AGV_ID)
					{
						itr = line->occuAgvs.erase(itr);
					}
					else {
						++itr;
					}
				}

				response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			}
		}
	}
	conn->send(response);
}