#include "mapmanager.h"
#include "../sqlite3/CppSQLite3.h"
#include "../msgprocess.h"
#include "../common.h"
#include "../taskmanager.h"
#include "../userlogmanager.h"

MapManager::MapManager() :mapModifying(false)
{
}

void MapManager::checkTable()
{
	//检查表
	try {
		if (!g_db.tableExists("agv_station")) {
			g_db.execDML("create table agv_station(id INTEGER, x INTEGER,y INTEGER,name char(64),type INTEGER,realX INTEGER,realY INTEGER,labelXoffset INTEGER,labelYoffset INTEGER,mapChange BOOL,locked BOOL);");
		}
		if (!g_db.tableExists("agv_line")) {
			g_db.execDML("create table agv_line(id INTEGER,name char(64),type INTEGER,start INTEGER,end INTEGER,p1x INTEGER,p1y INTEGER,p2x INTEGER,p2y INTEGER,length INTEGER,locked BOOL,direction INTEGER);");
		}
//        if (!g_db.tableExists("agv_line")) {
//            g_db.execDML("create table agv_line(id INTEGER,name char(64),type INTEGER,start INTEGER,end INTEGER,p1x INTEGER,p1y INTEGER,p2x INTEGER,p2y INTEGER,length INTEGER,locked BOOL,direction INTEGER);");
//        }
//        if (!g_db.tableExists("agv_line")) {
//            g_db.execDML("create table agv_line(id INTEGER,name char(64),type INTEGER,start INTEGER,end INTEGER,p1x INTEGER,p1y INTEGER,p2x INTEGER,p2y INTEGER,length INTEGER,locked BOOL,direction INTEGER);");
//        }
//        if (!g_db.tableExists("agv_line")) {
//            g_db.execDML("create table agv_line(id INTEGER,name char(64),type INTEGER,start INTEGER,end INTEGER,p1x INTEGER,p1y INTEGER,p2x INTEGER,p2y INTEGER,length INTEGER,locked BOOL,direction INTEGER);");
//        }
	}
	catch (CppSQLite3Exception &e) {
		LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
		return;
	}
	catch (std::exception e) {
		LOG(ERROR) << e.what();
		return;
	}
}

AgvLinePtr MapManager::getReverseLine(AgvLinePtr line)
{
	if (m_reverseLines.find(line) == m_reverseLines.end())return nullptr;
	return m_reverseLines[line];
}

AgvLinePtr MapManager::getLineById(int id)
{
	for (auto l : m_lines) {
		if (l->getId() == id)return l;
	}
	return nullptr;
}
AgvLinePtr MapManager::getLineByStartEnd(int start, int end)
{
	for (auto l : m_lines) {
		if (l->getStart() == start && l->getEnd() == end)
			return l;
	}
	return nullptr;
}
AgvStationPtr MapManager::getStationById(int id)
{
	for (auto s : m_stations) {
		if (s->getId() == id)return s;
	}
	return nullptr;
}
//载入地图
bool MapManager::load()
{
	checkTable();
	//载入数据
	mapModifying = true;
	if (!loadFromDb())
	{
		mapModifying = false;
		return false;
	}

	mapModifying = false;
	return true;
}

//保存到数据库
bool MapManager::save()
{
	try {
		checkTable();

		g_db.execDML("delete from agv_station;");

		g_db.execDML("delete from agv_line;");

		g_db.execDML("begin transaction;");
		char buf[SQL_MAX_LENGTH];
		for (auto station : m_stations) {
			snprintf(buf, SQL_MAX_LENGTH, "insert into agv_station values (%d, %d,%d,%s);", station->getId(), station->getX(), station->getY(), station->getName().c_str());
			g_db.execDML(buf);
		}
		for (auto line : m_lines) {
			snprintf(buf, SQL_MAX_LENGTH, "insert into agv_line values (%d,%d,%d,%d);", line->getId(), line->getStart(), line->getEnd(), line->getLength());
			g_db.execDML(buf);
		}
		g_db.execDML("commit transaction;");
	}
	catch (CppSQLite3Exception &e) {
		LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
		return false;
	}
	catch (std::exception e) {
		LOG(ERROR) << e.what();
		return false;
	}
	return true;
}

//从数据库中载入地图
bool MapManager::loadFromDb()
{
	try {
		checkTable();

        CppSQLite3Table table_station = g_db.getTable("select id, name, x ,y ,type ,realX ,realY ,labelXoffset ,labelYoffset ,mapChange ,locked  from agv_station;");
		if (table_station.numRows() > 0 && table_station.numFields() != 11)return false;
		for (int row = 0; row < table_station.numRows(); row++)
		{
			table_station.setRow(row);

			int id = atoi(table_station.fieldValue(0));
			std::string name = std::string(table_station.fieldValue(1));
			int x = atoi(table_station.fieldValue(2));
			int y = atoi(table_station.fieldValue(3));			
			int type = atoi(table_station.fieldValue(4));
			int realX = atoi(table_station.fieldValue(5));
			int realY = atoi(table_station.fieldValue(6));
			int labeXoffset = atoi(table_station.fieldValue(7));
			int labelYoffset = atoi(table_station.fieldValue(8));
			bool mapChanged = atoi(table_station.fieldValue(9)) == 1;
			bool locked = atoi(table_station.fieldValue(10)) == 1;

			AgvStationPtr station = AgvStationPtr(new AgvStation(id, name, (MapPoint::Map_Point_Type)type, x, y, realX, realY, labeXoffset, labelYoffset, mapChanged, locked));

			m_stations.push_back(station);
		}

		CppSQLite3Table table_line = g_db.getTable("select id,name,type,start,end,p1x,p1y,p2x,p2y,length,locked,direction from agv_line;");
		if (table_line.numRows() > 0 && table_line.numFields() != 12)return false;
		for (int row = 0; row < table_line.numRows(); row++)
		{
			table_line.setRow(row);

			int id = atoi(table_station.fieldValue(0));
			std::string name = std::string(table_station.fieldValue(1));
			int type = atoi(table_station.fieldValue(2));
			int start = atoi(table_station.fieldValue(3));
			int end = atoi(table_station.fieldValue(4));
			int p1x = atoi(table_station.fieldValue(5));
			int p1y = atoi(table_station.fieldValue(6));
			int p2x = atoi(table_station.fieldValue(7));
			int p2y = atoi(table_station.fieldValue(8));
			int length = atoi(table_station.fieldValue(9));
            bool locked = atoi(table_station.fieldValue(10)) == 1;
			
            AgvLinePtr line = AgvLinePtr(new AgvLine(id, name, start, end, (MapPath::Map_Path_Type)type, length, p1x, p1y, p2x, p2y, locked));
			m_lines.push_back(line);
		}

		getReverseLines();
		getAdj();
	}
	catch (CppSQLite3Exception &e) {
		LOG(ERROR) << e.errorCode() << ":" << e.errorMessage();
		return false;
	}
	catch (std::exception e) {
		LOG(ERROR) << e.what();
		return false;
	}
	return true;
}


//获取最优路径
std::vector<int> MapManager::getBestPath(int agv, int lastStation, int startStation, int endStation, int &distance, bool changeDirect)
{
	distance = DISTANCE_INFINITY;
	if (mapModifying) return std::vector<int>();
	int disA = DISTANCE_INFINITY;
	int disB = DISTANCE_INFINITY;

	std::vector<int> a = getPath(agv, lastStation, startStation, endStation, disA, false);
	std::vector<int> b;
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

std::vector<int> MapManager::getPath(int agv, int lastStation, int startStation, int endStation, int &distance, bool changeDirect)
{

	//TODO:
	std::vector<int> result;

	//distance = DISTANCE_INFINITY;

	//if (lastStation <= 0) lastStation = startStation;
	//if (lastStation <= 0)return result;
	//if (startStation <= 0)return result;
	//if (endStation <= 0)return result;

	//AgvStationPtr startStationPtr = getStationById(startStation);
	//if (startStationPtr == nullptr)return result;
	//AgvStationPtr endStationPtr = getStationById(endStation);
	//if (endStationPtr == nullptr)return result;


	//if (startStationPtr->getOccuAgv() > 0 && startStationPtr->getOccuAgv() != agv)return result;
	//if (endStationPtr->getOccuAgv() > 0 && endStationPtr->getOccuAgv() != agv)return result;

	//if (startStation == endStation) {
	//	if (changeDirect && lastStation != startStation) {
	//		for (auto line : m_lines) {
	//			if (line->getStart() == lastStation && line->getEnd() == startStation) {
	//				result.push_back(line->getId());
	//				distance = line->getLength();
	//			}
	//		}
	//	}
	//	else {
	//		distance = 0;
	//	}
	//	return result;
	//}

	//std::multimap<int, AgvLinePtr> Q;//key -->  distance ;value --> station;

	//for (auto line : m_lines) {
	//	line->father = NULL;
	//	line->distance = DISTANCE_INFINITY;
	//	line->color = AGV_LINE_COLOR_WHITE;
	//}

	////增加一种通行的判定：
	////同事AGV2 要从 C点 到达 D点，同事路过B点。
	////如果AGV1 要从 A点 到达 B点。如果AGV1先到达B点，会导致AGV2 无法继续运行。
	////判定终点的线路 是否占用
	////endPoint是终点，lastPoint是到达endPoint的上一站。
	//{
	//	for (auto templine : m_lines) {
	//		if (templine->getStart() == endStation) {
	//			if (templine->getOccuAgvs().size() > 1 || (templine->getOccuAgvs().size() == 1 && (*(templine->getOccuAgvs().begin())) != agv)) {
	//				//TODO:该方式到达这个地方 不可行.该线路 置黑、
	//				AgvLinePtr  llid = m_reverseLines[templine];
	//				llid->color = AGV_LINE_COLOR_BLACK;
	//				llid->distance = DISTANCE_INFINITY;
	//			}
	//		}
	//	}
	//}


	//if (lastStation == startStation) {
	//	for (auto l : m_lines) {
	//		if (l->getStart() == lastStation) {
	//			AgvLinePtr reverse = m_reverseLines[l];
	//			if (reverse->getOccuAgvs().size() == 0 && (l->getEnd()->getOccuAgv() == nullptr || l->getEnd()->getOccuAgv() == agv)) {
	//				if (l->color == AGV_LINE_COLOR_BLACK)continue;
	//				l->distance = l->getLength();
	//				l->color = AGV_LINE_COLOR_GRAY;
	//				Q.insert(std::make_pair(l->distance, l));
	//			}
	//		}
	//	}
	//}
	//else {
	//	for (auto line : m_lines) {
	//		if (line->getStart() == lastStation && line->getEnd() == lastStation) {
	//			AgvLinePtr reverse = m_reverseLines[line];
	//			if (reverse->getOccuAgvs().size() == 0 && (line->getEnd()->getOccuAgv() == nullptr || line->getEnd()->getOccuAgv() == agv)) {
	//				if (line->color == AGV_LINE_COLOR_BLACK)continue;
	//				line->distance = 0;
	//				line->color = AGV_LINE_COLOR_GRAY;
	//				Q.insert(std::make_pair(line->distance, line));
	//				break;
	//			}
	//		}
	//	}
	//}

	//while (Q.size() > 0) {
	//	auto front = Q.begin();
	//	AgvLinePtr startLine = front->second;

	//	if (m_adj.find(startLine) == m_adj.end()) {
	//		startLine->color = AGV_LINE_COLOR_BLACK;
	//		for (auto ll = Q.begin(); ll != Q.end();) {
	//			if (ll->second == startLine) {
	//				ll = Q.erase(ll);
	//			}
	//			else {
	//				++ll;
	//			}
	//		}
	//		continue;
	//	}
	//	for (auto line : m_adj[startLine]) {
	//		if (line->color == AGV_LINE_COLOR_BLACK)continue;
	//		if (line->color == AGV_LINE_COLOR_WHITE) {
	//			AgvLinePtr reverse = m_reverseLines[line];
	//			if (reverse->getOccuAgvs().size() == 0 && (line->getEnd()->getOccuAgv() == nullptr || line->getEnd()->getOccuAgv() == agv)) {
	//				line->distance = startLine->distance + line->getLength();
	//				line->color = AGV_LINE_COLOR_GRAY;
	//				line->father = startLine;
	//				Q.insert(std::make_pair(line->distance, line));
	//			}
	//		}
	//		else if (line->color == AGV_LINE_COLOR_GRAY) {
	//			if (line->distance > startLine->distance + line->getLength()) {
	//				AgvLinePtr reverse = m_reverseLines[line];
	//				if (reverse->getOccuAgvs().size() == 0 && (line->getEnd()->getOccuAgv() == nullptr || line->getEnd()->getOccuAgv() == agv)) {
	//					int old_distance = line->distance;
	//					line->distance = startLine->distance + line->getLength();
	//					line->father = startLine;
	//					//删除旧的
	//					for (auto iiitr = Q.begin(); iiitr != Q.end();) {
	//						if (iiitr->second == line && iiitr->first == old_distance) {
	//							iiitr = Q.erase(iiitr);
	//						}
	//					}
	//					//加入新的
	//					Q.insert(std::make_pair(line->distance, line));
	//				}
	//			}
	//		}
	//	}

	//	startLine->color = AGV_LINE_COLOR_BLACK;
	//	for (auto ll = Q.begin(); ll != Q.end();) {
	//		if (ll->second == startLine && ll->first == startLine->distance) {
	//			ll = Q.erase(ll);
	//		}
	//		else {
	//			++ll;
	//		}
	//	}
	//}

	//int index = 0;
	//int minDis = DISTANCE_INFINITY;

	//for (auto ll : m_lines) {
	//	if (ll->getEnd() == endStation) {
	//		if (ll->distance < minDis) {
	//			minDis = ll->distance;
	//			index = ll;
	//		}
	//	}
	//}

	//distance = minDis;

	//while (true) {
	//	if (index == 0)break;
	//	result.push_back(index);
	//	index = index->father;
	//}
	//std::reverse(result.begin(), result.end());

	//if (result.size() > 0 && lastStation != startStation) {
	//	if (!changeDirect) {
	//		int  agv_line = *(result.begin());
	//		if (agv_line->getStart() == lastStation && agv_line->getEnd() == startStation) {
	//			result.erase(result.begin());
	//		}
	//	}
	//}

	return result;
}


void MapManager::getReverseLines()
{
	for (auto a : m_lines) {
		for (auto b : m_lines) {
			if (a->getId() == b->getId())continue;
			if (a->getStart() == b->getEnd() && a->getEnd() == b->getStart()) {
				m_reverseLines[a] = b;
			}
		}
	}
}

void MapManager::getAdj()
{
	for (auto a : m_lines) {
		for (auto b : m_lines) {
			if (a == b)continue;
			if (a->getEnd() == b->getStart() && a->getEnd() != b->getStart()) {
				if (m_adj.find(a) != m_adj.end()) {
					m_adj[a].push_back(b);
				}
				else {
					std::vector<AgvLinePtr > ll;
					ll.push_back(b);
					m_adj.insert(std::make_pair(a, ll));
				}
			}
		}
	}
}

void MapManager::clear()
{
	m_reverseLines.clear();
	m_adj.clear();
	m_reverseLines.clear();
	m_blocks.clear();
	m_stations.clear();
	m_lines.clear();
	
}

void MapManager::interSetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (TaskManager::getInstance()->hasTaskDoing())
	{
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_TASKING;
	}
	else {
		UserLogManager::getInstance()->push(conn->getUserName() + " set map");

		mapModifying = true;
		clear();
		try {
			g_db.execDML("delete from agv_station;");
			g_db.execDML("delete from agv_line;");
			g_db.execDML("delete from agv_bkg");
			g_db.execDML("delete from agv_block");
			g_db.execDML("delete from agv_group_group");
			g_db.execDML("delete from agv_group_agv");

			//TODO: 
			//1.解析站点
			for (int i = 0; i < request["stations"].size(); ++i)
			{
				Json::Value station = request["stations"][i];
				int id = station["id"].asInt();
				std::string name = station["name"].asString();
				int station_type = station["point_type"].asInt();
				int x = station["x"].asInt();
				int y = station["y"].asInt();
				int realX = station["realX"].asInt();
				int realY = station["realY"].asInt();
				int labelXoffset = station["labelXoffset"].asInt();
				int labelYoffset = station["labelYoffset"].asInt();
				bool mapchanged = station["mapchanged"].asBool();
				bool locked = station["locked"].asBool();
				//TODO:

				//TODO:


			}

			//2.解析线路

			//3.解析楼层

			//4.解析背景图片

			//5.解析block

			//6.解析group

			//7.解析背景图片


		}
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			LOG(ERROR) << "sqlerr code:" << e.what();
		}
	}

	mapModifying = false;

	conn->send(response);
}

//void MapManager::interCreateAddStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
//{
//	Json::Value response;
//	response["type"] = MSG_TYPE_RESPONSE;
//	response["todo"] = request["todo"];
//	response["queuenumber"] = request["queuenumber"];
//	response["result"] = RETURN_MSG_RESULT_SUCCESS;
//
//    if (!mapModifying) {
//        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
//        snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
//    }
//    else {
//        if (msg.head.body_length != sizeof(STATION_INFO)) {
//            response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
//            snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error STATION_INFO length");
//        }
//        else {
//            STATION_INFO station;
//            memcpy_s(&station, sizeof(STATION_INFO),msg.body, sizeof(STATION_INFO));
//            char buf[MSG_LONG_LONG_STRING_LEN];
//            snprintf(buf,MSG_LONG_LONG_STRING_LEN, "insert into agv_station values (%d, %d,%d,%s);", station.id, station.x,station.y,station.name);
//            try{
//                g_db.execDML(buf);
//                response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
//            }
//			catch (CppSQLite3Exception e) {
//				response["result"] = RETURN_MSG_RESULT_FAIL;
//				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
//				std::stringstream ss;
//				ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
//				response["error_info"] = ss.str();
//				LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
//			}
//			catch (std::exception e) {
//				response["result"] = RETURN_MSG_RESULT_FAIL;
//				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
//				std::stringstream ss;
//				ss << "info:" << e.what();
//				response["error_info"] = ss.str();
//				LOG(ERROR) << "sqlerr code:" << e.what();
//			}
//        }
//    }
//    conn->send(response);
//}
//
//void MapManager::interCreateAddLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
//{
//	Json::Value response;
//	response["type"] = MSG_TYPE_RESPONSE;
//	response["todo"] = request["todo"];
//	response["queuenumber"] = request["queuenumber"];
//	response["result"] = RETURN_MSG_RESULT_SUCCESS;
//
//    if (!mapModifying) {
//        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
//        snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
//    }
//    else {
//        if (msg.head.body_length != sizeof(AGV_LINE)) {
//            response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
//            snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","error AGV_LINE length");
//        }
//        else {
//            AGV_LINE line;
//            memcpy_s(&line,sizeof(AGV_LINE), msg.body, sizeof(AGV_LINE));
//            char buf[SQL_MAX_LENGTH];
//            snprintf(buf,SQL_MAX_LENGTH, "INSERT INTO agv_line (id,line_startStation,line_endStation,line_length) VALUES (%d,%d,%d,%d);", line.id, line.startStation,line.endStation,line.length);
//            try{
//                g_db.execDML(buf);
//                response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
//            }
//			catch (CppSQLite3Exception e) {
//				response["result"] = RETURN_MSG_RESULT_FAIL;
//				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
//				std::stringstream ss;
//				ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
//				response["error_info"] = ss.str();
//				LOG(ERROR) << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
//			}
//			catch (std::exception e) {
//				response["result"] = RETURN_MSG_RESULT_FAIL;
//				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
//				std::stringstream ss;
//				ss << "info:" << e.what();
//				response["error_info"] = ss.str();
//				LOG(ERROR) << "sqlerr code:" << e.what();
//			}
//
//            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
//        }
//
//    }
//    conn->send(response);
//}
//
//void MapManager::interCreateFinish(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
//{
//	Json::Value response;
//	response["type"] = MSG_TYPE_RESPONSE;
//	response["todo"] = request["todo"];
//	response["queuenumber"] = request["queuenumber"];
//	response["result"] = RETURN_MSG_RESULT_SUCCESS;
//
//    if (!mapModifying) {
//        response.return_head.error_code = RETURN_MSG_ERROR_CODE_NOT_CTREATING;
//        snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","is not creating map");
//    }
//    else {
//        UserLogManager::getInstance()->push(conn->getUserName()+"重新设置地图完成");
//        if(!save()){
//            response.return_head.result = RETURN_MSG_RESULT_FAIL;
//            response.return_head.error_code = RETURN_MSG_ERROR_CODE_SAVE_SQL_FAIL;
//            snprintf(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","save data to sql fail");
//        }else{
//            getReverseLines();
//            getAdj();
//            mapModifying = false;
//            response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
//        }
//    }
//    conn->send(response);
//
//    //通知所有客户端，map更新了
//    MsgProcess::getInstance()->notifyAll(ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE);
//}

void MapManager::interGetMap(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (mapModifying) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
	}
	else {
		UserLogManager::getInstance()->push(conn->getUserName() + " get map");
		/*response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
		Json::Value agv_stations;
		for (auto s : m_stations) {
			Json::Value info;
			info["id"]= s->getId();
			info["x"] = s->getX();
			info["y"] = s->getY();
			info["floorId"] = s->getFloorId();
			info["occuagv"] = s->getOccuAgv.getId();;
			if(s->getOccuAgv()!=nullptr)
				info.occuagv = s->getOccuAgv()->getId();
			snprintf(info.name,MSG_STRING_LEN,s->getName().c_str(),s->getName().length());
			memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&info,sizeof(STATION_INFO));
			response.head.flag = 1;
			response.head.body_length = sizeof(STATION_INFO);
			conn->send(response);
		}
		response.head.body_length = 0;
		response.head.flag = 0;*/
	}
	conn->send(response);
}

//void MapManager::interListLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
//{
//	Json::Value response;
//	response["type"] = MSG_TYPE_RESPONSE;
//	response["todo"] = request["todo"];
//	response["queuenumber"] = request["queuenumber"];
//	response["result"] = RETURN_MSG_RESULT_SUCCESS;
//
//    if (mapModifying) {
//		response["result"] = RETURN_MSG_RESULT_FAIL;
//		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
//    }
//    else {
//        UserLogManager::getInstance()->push(conn->getUserName()+"获取地图线路信息");
//        response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
//        for (auto l : m_lines) {
//            AGV_LINE line;
//            line.id = l->getId();
//            line.startStation = l->getStartStation()->getId();
//            line.endStation = l->getEndStation()->getId();
//            line.length = l->getLength();
//
//            memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&line,sizeof(AGV_LINE));
//
//            response.head.flag = 1;
//            response.head.body_length = sizeof(AGV_LINE);
//            conn->send(response);
//        }
//        response.head.flag = 0;
//        response.head.body_length = 0;
//    }
//    conn->send(response);
//}

void MapManager::interTrafficControlStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (mapModifying) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
	}
	else {
		//if (msg.head.body_length != sizeof(int32_t)) {
  //          response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
  //          snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error station id length");
		//}
		//else {
		//	int stationId = 0;
		//	memcpy_s(&stationId, sizeof(int32_t), msg.body, sizeof(int32_t));
		//	AgvStationPtr station = getStationById(stationId);
		//	if (station == nullptr) {
		//		response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
  //              snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded station id");
		//	}
		//	else {
		//		//存库...
		//		//TODO

		//		//设置
  //              station->setLocked(true);
		//		response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
		//	}			
		//}
	}
	conn->send(response);
}

void MapManager::interTrafficReleaseLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (mapModifying) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
	}
	else {
		//      if (request.head.body_length != sizeof(int32_t)) {
		//          response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
		//          snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error line id length");
			  //}
			  //else {
			  //	int lineId = 0;
		//          memcpy_s(&lineId, sizeof(int32_t), request.body, sizeof(int32_t));
			  //	AgvLinePtr line = getLineById(lineId);
			  //	if (line == nullptr) {
			  //		response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
		//              snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded line id");
			  //	}
			  //	else {
			  //		//存库...
			  //		//TODO
			  //		//设置
			  //		//是否已经被管制了
		//              line->setLocked(false);
			  //		response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			  //	}
			  //}
	}
	conn->send(response);
}


void MapManager::interTrafficReleaseStation(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (mapModifying) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
	}
	else {
		/*if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
			snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error station id length");
		}
		else {*/
		//int stationId = 0;
		//memcpy_s(&stationId, sizeof(int32_t), msg.body, sizeof(int32_t));
		//AgvStationPtr station = getStationById(stationId);
		//if (station == nullptr) {
		//	response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
//             snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded station id");
		 //}
		 //else {
		 //	//存库...
		 //	//TODO

		 //	//设置
//             if (station->getOccuAgv() != nullptr/* && station->getOccuAgv()->getId() == TRAFFIC_OCCUR_AGV_ID*/) {
//                 station->setOccuAgv(nullptr);
		 //	}
		 //	response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
		 //}
	 //}
	}
	conn->send(response);
}

void MapManager::interTrafficControlLine(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (mapModifying) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_CTREATING;
	}
	else {
		/*if (msg.head.body_length != sizeof(int32_t)) {
			response.return_head.error_code = RETURN_MSG_ERROR_CODE_PARAMS;
			snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "error line id length");
		}
		else {
			int lineId = 0;
			memcpy_s(&lineId, sizeof(int32_t), msg.body, sizeof(int32_t));
			AgvLinePtr line = getLineById(lineId);
			if (line == nullptr) {
				response.return_head.error_code = RETURN_MSG_ERROR_CODE_UNFINDED;
				snprintf(response.return_head.error_info, MSG_LONG_STRING_LEN, "%s", "unfinded line id");
			}
			else {
				line->setLocked(true);
				response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
			}
		}*/
	}
	conn->send(response);
}
