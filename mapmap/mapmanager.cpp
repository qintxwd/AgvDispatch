#include "mapmanager.h"
#include "../sqlite3/CppSQLite3.h"
#include "../msgprocess.h"
#include "../common.h"
#include "../taskmanager.h"
#include "../userlogmanager.h"
#include "../base64.h"
#include "blockmanager.h"
#include <algorithm>

MapManager::MapManager() :mapModifying(false)
{
}

void MapManager::checkTable()
{
    //检查表
    try {
        if (!g_db.tableExists("agv_station")) {
            g_db.execDML("create table agv_station(id INTEGER,name char(64),type INTEGER, x INTEGER,y INTEGER,realX INTEGER,realY INTEGER,realA INTEGER DEFAULT 0, labelXoffset INTEGER,labelYoffset INTEGER,mapChange BOOL,locked BOOL,ip char(64),port INTEGER,agvType INTEGER,lineId char(64));");
        }
        if (!g_db.tableExists("agv_line")) {
            g_db.execDML("create table agv_line(id INTEGER,name char(64),type INTEGER,start INTEGER,end INTEGER,p1x INTEGER,p1y INTEGER,p2x INTEGER,p2y INTEGER,length INTEGER,locked BOOL,speed DOUBLE DEFAULT 0.4);");
        }
        if (!g_db.tableExists("agv_bkg")) {
            g_db.execDML("create table agv_bkg(id INTEGER,name char(64),data blob,data_len INTEGER,x INTEGER,y INTEGER,width INTEGER,height INTEGER,filename char(512));");
        }
        if (!g_db.tableExists("agv_floor")) {
            g_db.execDML("create table agv_floor(id INTEGER,name char(64),point INTEGER,path INTEGER,bkg INTEGER,originX INTEGER,originY INTEGER,rate DOUBLE);");
        }
        if (!g_db.tableExists("agv_block")) {
            g_db.execDML("create table agv_block(id INTEGER,name char(64),spirits char(512));");
        }
        if (!g_db.tableExists("agv_group")) {
            g_db.execDML("create table agv_group(id INTEGER,name char(64),spirits char(512));");
        }
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        return;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}", e.what());
        return;
    }
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

MapSpirit *MapManager::getMapSpiritById(int id)
{
    return g_onemap.getSpiritById(id);
}

MapSpirit *MapManager::getMapSpiritByName(std::string name)
{
    auto ae = g_onemap.getAllElement();
    for (auto e : ae) {
        if (e->getName() == name)return e;
    }
    return nullptr;
}

MapPath *MapManager::getMapPathByStartEnd(int start, int end)
{
    auto ae = g_onemap.getAllElement();
    for (auto e : ae) {
        if (e->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
            MapPath *path = static_cast<MapPath *>(e);
            if (path->getStart() == start && path->getEnd() == end)return path;
        }
    }
    return nullptr;
}

void MapManager::printGroup()
{
    UNIQUE_LCK(groupMtx);
    for(auto p=group_occuagv.begin();p!=group_occuagv.end();++p){
        combined_logger->debug("groupId:{0} ====================",p->first);
        auto pp = p->second;
        combined_logger->debug("agv:{0}",pp.first);
        std::stringstream ss1;
        ss1<<"stations: ";
        for(auto ppp:pp.second){
            auto ptr = getPointById(ppp);
            if(ptr!=nullptr){
                ss1<<ptr->getName()<<" ";
            }
        }
        std::stringstream ss2;
        ss2<<"paths: ";
        for(auto ppp:pp.second){
            auto ptr = getPathById(ppp);
            if(ptr!=nullptr){
                ss2<<ptr->getName()<<" ";
            }
        }
        combined_logger->debug("{0}",ss1.str());
        combined_logger->debug("{0}",ss2.str());
        combined_logger->debug("===================");
    }
}

//一个Agv占领一个站点
void MapManager::addOccuStation(int station, AgvPtr occuAgv)
{
    station_occuagv[station] = occuAgv->getId();
    combined_logger->debug("occu station:{0} agv:{1}", station, occuAgv->getId());
    
    std::vector<int> groupIds = getGroup(station);

    for(auto groupId:groupIds)
    {
        combined_logger->info("occupy group:{0} agv:{1}", groupId, occuAgv->getId());
        UNIQUE_LCK(groupMtx);
        auto iter = group_occuagv.find(groupId);
        std::vector<int> occuSpirits;
        if(iter != group_occuagv.end())
        {
            occuSpirits = iter->second.second;
        }
        occuSpirits.push_back(station);
        group_occuagv[groupId] = std::make_pair(occuAgv->getId(), occuSpirits);
    }
}

//线路的反向占用//这辆车行驶方向和线路方向相反
void MapManager::addOccuLine(int line, AgvPtr occuAgv)
{
    if(std::find(line_occuagvs[line].begin(), line_occuagvs[line].end(), occuAgv->getId()) == line_occuagvs[line].end())
    {
        line_occuagvs[line].push_back(occuAgv->getId());
    }
    combined_logger->debug("occupy line:{0} agv:{1}", line, occuAgv->getId());

    //同一条line可能属于多个Group
    std::vector<int> groupIds = getGroup(line);
    for(auto groupId:groupIds)
    {
        UNIQUE_LCK(groupMtx);
        auto iter = group_occuagv.find(groupId);
        std::vector<int> occuSpirits;
        if(iter != group_occuagv.end())
        {
            occuSpirits = iter->second.second;
        }
        occuSpirits.push_back(line);
        group_occuagv[groupId] = std::make_pair(occuAgv->getId(), occuSpirits);
    }
}

//如果车辆占领该站点，释放
void MapManager::freeStation(int station, AgvPtr occuAgv)
{
    if (station_occuagv[station] == occuAgv->getId()) {
        station_occuagv[station] = 0;
    }
    //combined_logger->debug("free station:{0} agv:{1}", station, occuAgv->getId());

    std::vector<int> groupIds = getGroup(station);
    for(auto groupId:groupIds)
    {
        UNIQUE_LCK(groupMtx);
        auto iter = group_occuagv.find(groupId);
        std::vector<int> occuSpirits;
        if(iter != group_occuagv.end())
        {
            occuSpirits = iter->second.second;
            auto ii = std::remove(occuSpirits.begin(),occuSpirits.end(),station);
            occuSpirits.erase(ii,occuSpirits.end());
            if(!occuSpirits.size())
            {
                group_occuagv.erase(iter);
                combined_logger->info("free group:{0} agv:{1}", groupId, occuAgv->getId());
            }else{
                group_occuagv[groupId] = std::make_pair(occuAgv->getId(), occuSpirits);
            }
        }
    }
}

//如果车辆在线路的占领表中，释放出去
void MapManager::freeLine(int line, AgvPtr occuAgv)
{
    for (auto itr = line_occuagvs[line].begin(); itr != line_occuagvs[line].end(); ) {
        if (*itr == occuAgv->getId()) {
            itr = line_occuagvs[line].erase(itr);
        }
        else {
            ++itr;
        }
    }

    //combined_logger->info("free line:{0} agv:{1}", line, occuAgv->getId());

    //TODO 如果一段路径同属于多个block 此处需要验证
    std::vector<int> groupIds = getGroup(line);
    for(auto groupId:groupIds)
    {
        UNIQUE_LCK(groupMtx);
        auto iter = group_occuagv.find(groupId);
        std::vector<int> occuSpirits;
        if(iter != group_occuagv.end())
        {
            occuSpirits = iter->second.second;
            auto ii = std::remove(occuSpirits.begin(),occuSpirits.end(),line);
            occuSpirits.erase(ii,occuSpirits.end());
            if(!occuSpirits.size())
            {
                group_occuagv.erase(iter);
                combined_logger->info("free group:{0} agv:{1}", groupId, occuAgv->getId());
            }else{
                group_occuagv[groupId] = std::make_pair(occuAgv->getId(), occuSpirits);
            }
        }
    }
}

//获得站点楼层
int MapManager::getStationFloor(int station)
{
    std::list<MapFloor *> floors = g_onemap.getFloors();//楼层
    for(auto f:floors){
        std::list<int> fps = f->getPoints();
        for(auto fp:fps){
            if(station == fp){
                return f->getId();
            }
        }
    }
    return -1;
}
//是否同一楼层站点

//是否同一楼层站点
bool MapManager::isSameFloorStation(int station_1, int station_2)
{
    int floor_1 = getStationFloor(station_1);
    int floor_2 = getStationFloor(station_2);

    std::cout<<"是否同一楼层站点, floor_1 : " << floor_1 <<std::endl;
    std::cout<<"是否同一楼层站点, floor_2 : " << floor_2 <<std::endl;

    if(floor_1 > 0  && floor_1 == floor_2 )
        return true;
    else
        return false;
}

//bool MapManager::tryAddBlockOccu(std::vector<int> blocks, int agvId, int spiritId)
//{
//    if(blocks.size()>0)
//    {
//        UNIQUE_LCK(blockMtx);
//        for(auto block:blocks){
//            auto iter = block_occuagv.find(block);
//            if(iter != block_occuagv.end() && iter->second.first != agvId)
//                return false;
//        }
//        combined_logger->debug("agv{0} occu blocks with {1} ",agvId,getMapSpiritById(spiritId)->getName());
//        for(auto block:blocks){
//            auto iter = block_occuagv.find(block);
//            std::vector<int> occuSpirits;
//            if(iter != block_occuagv.end())
//            {
//                occuSpirits = iter->second.second;
//            }
//            if(std::find(occuSpirits.begin(),occuSpirits.end(),spiritId) == occuSpirits.end()){
//                occuSpirits.push_back(spiritId);
//                block_occuagv[block] = std::make_pair(agvId, occuSpirits);
//            }
//        }
//    }
//    return true;
//}

//void MapManager::freeBlockOccu(std::vector<int> blocks, int agvId, int spiritId)
//{
//    if(blocks.size()>0)
//    {
//        UNIQUE_LCK(blockMtx);
//        combined_logger->debug("agv{0} free blocks with {1} ",agvId,getMapSpiritById(spiritId)->getName());

//        for(auto block:blocks)
//        {
//            auto iter = block_occuagv.find(block);
//            if(iter != block_occuagv.end())
//            {
//                auto occuSpirits = iter->second.second;
//                auto ii = std::remove(occuSpirits.begin(),occuSpirits.end(),spiritId);
//                occuSpirits.erase(ii,occuSpirits.end());

//                if(occuSpirits.size()<=0)
//                {
//                    block_occuagv.erase(iter);
//                    combined_logger->info("free block:{0} agv:{1}", block, agvId);
//                }else{
//                    block_occuagv[block] = std::make_pair(agvId, occuSpirits);
//                }
//            }
//        }
//    }
//}

//保存到数据库
bool MapManager::save()
{
    try {
        checkTable();

        g_db.execDML("delete from agv_station;");

        g_db.execDML("delete from agv_line;");

        g_db.execDML("delete from agv_bkg;");

        g_db.execDML("delete from agv_floor;");

        g_db.execDML("delete from agv_block;");

        g_db.execDML("delete from agv_group;");

        g_db.execDML("begin transaction;");

        std::list<MapSpirit *> spirits = g_onemap.getAllElement();

        CppSQLite3Buffer bufSQL;

        for (auto spirit : spirits) {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point) {
                MapPoint *station = static_cast<MapPoint *>(spirit);
                bufSQL.format("insert into agv_station(id,name,type, x,y,realX,realY,realA,labelXoffset,labelYoffset ,mapChange,locked,ip,port,agvType,lineId) values (%d, '%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s',%d,%d,'%s');", station->getId(), station->getName().c_str(), station->getPointType(), station->getX(), station->getY(),
                              station->getRealX(), station->getRealY(), station->getRealA(), station->getLabelXoffset(), station->getLabelYoffset(), station->getMapChange(), station->getLocked(), station->getIp().c_str(), station->getPort(), station->getAgvType(),station->getLineId().c_str());
                g_db.execDML(bufSQL);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
                MapPath *path = static_cast<MapPath *>(spirit);
                bufSQL.format("insert into agv_line(id ,name,type ,start ,end ,p1x ,p1y ,p2x ,p2y ,length ,locked,speed) values (%d,'%s', %d,%d, %d, %d, %d, %d, %d, %d, %d,%.2f);", path->getId(), path->getName().c_str(), path->getPathType(), path->getStart(), path->getEnd(),
                              path->getP1x(), path->getP1y(), path->getP2x(), path->getP2y(), path->getLength(), path->getLocked(),path->getSpeed());
                g_db.execDML(bufSQL);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Background) {
                MapBackground *bkg = static_cast<MapBackground *>(spirit);
                CppSQLite3Binary blob;
                blob.setBinary((unsigned char *)bkg->getImgData(), bkg->getImgDataLen());
                bufSQL.format("insert into agv_bkg(id ,name,data,data_len,x ,y ,width ,height ,filename) values (%d,'%s',%Q, %d,%d, %d, %d, %d,'%s');", bkg->getId(), bkg->getName().c_str(), blob.getEncoded(), bkg->getImgDataLen(), bkg->getX(), bkg->getY(), bkg->getWidth(), bkg->getHeight(), bkg->getFileName().c_str());
                g_db.execDML(bufSQL);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Floor) {
                MapFloor *floor = static_cast<MapFloor *>(spirit);
                std::stringstream pointstr;
                std::list<int> ps = floor->getPoints();
                for (auto p : ps) {
                    pointstr << p << ";";
                }
                std::stringstream pathstr;
                std::list<int> pas = floor->getPaths();
                for (auto pa : pas) {
                    pathstr << pa << ";";
                }
                bufSQL.format("insert into agv_floor(id ,name,point,path,bkg,originX,originY,rate) values (%d,'%s', '%s','%s',%d,%d,%d,%lf);", floor->getId(), floor->getName().c_str(), pointstr.str().c_str(), pathstr.str().c_str(), floor->getBkg(), floor->getOriginX(), floor->getOriginY(), floor->getRate());
                g_db.execDML(bufSQL);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Block) {
                MapBlock *block = static_cast<MapBlock *>(spirit);
                std::stringstream str;
                std::list<int> ps = block->getSpirits();
                for (auto p : ps)str << p << ";";
                bufSQL.format("insert into agv_block(id ,name,spirits) values (%d,'%s', '%s');", block->getId(), block->getName().c_str(), str.str().c_str());
                g_db.execDML(bufSQL);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Group) {
                MapGroup *group = static_cast<MapGroup *>(spirit);

                std::stringstream str1;
                std::list<int> ps1 = group->getSpirits();
                for (auto p : ps1)str1 << p << ";";

                bufSQL.format("insert into agv_group(id ,name,spirits,groupType) values (%d,'%s', '%s',%d);", group->getId(), group->getName().c_str(), str1.str().c_str(), group->getGroupType());
                g_db.execDML(bufSQL);
            }
        }

        g_db.execDML("commit transaction;");
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        return false;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}", e.what());
        return false;
    }
    return true;
}

//从数据库中载入地图
bool MapManager::loadFromDb()
{
    try {
        checkTable();

        CppSQLite3Table table_station = g_db.getTable("select id, name, x ,y ,type ,realX ,realY ,realA, labelXoffset ,labelYoffset ,mapChange ,locked ,ip ,port ,agvType ,lineId  from agv_station;");
        if (table_station.numRows() > 0 && table_station.numFields() != 16)
        {
            combined_logger->error("MapManager loadFromDb agv_station error!");
            return false;
        }
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
            int realA = atoi(table_station.fieldValue(7));
            int labeXoffset = atoi(table_station.fieldValue(8));
            int labelYoffset = atoi(table_station.fieldValue(9));
            bool mapchange = atoi(table_station.fieldValue(10)) == 1;
            bool locked = atoi(table_station.fieldValue(11)) == 1;
            std::string ip = std::string(table_station.fieldValue(12));
            int port = atoi(table_station.fieldValue(13));
            int agvType = atoi(table_station.fieldValue(14));
            std::string lineId = std::string(table_station.fieldValue(15));

            MapPoint::Map_Point_Type t = static_cast<MapPoint::Map_Point_Type>(type);
            MapPoint *point = new MapPoint(id, name, t, x, y, realX, realY, realA, labeXoffset, labelYoffset, mapchange, locked, ip, port, agvType, lineId);

            g_onemap.addSpirit(point);
        }

        CppSQLite3Table table_line = g_db.getTable("select id,name,type,start,end,p1x,p1y,p2x,p2y,length,locked,speed from agv_line;");
        if (table_line.numRows() > 0 && table_line.numFields() != 12)return false;
        for (int row = 0; row < table_line.numRows(); row++)
        {
            table_line.setRow(row);

            int id = atoi(table_line.fieldValue(0));
            std::string name = std::string(table_line.fieldValue(1));
            int type = atoi(table_line.fieldValue(2));
            int start = atoi(table_line.fieldValue(3));
            int end = atoi(table_line.fieldValue(4));
            int p1x = atoi(table_line.fieldValue(5));
            int p1y = atoi(table_line.fieldValue(6));
            int p2x = atoi(table_line.fieldValue(7));
            int p2y = atoi(table_line.fieldValue(8));
            int length = atoi(table_line.fieldValue(9));
            bool locked = atoi(table_line.fieldValue(10)) == 1;
            double speed = atof(table_line.fieldValue(11));

            MapPath::Map_Path_Type t = static_cast<MapPath::Map_Path_Type>(type);
            MapPath *path = new MapPath(id, name, start, end, t, length, p1x, p1y, p2x, p2y, locked,speed);
            g_onemap.addSpirit(path);
        }

        CppSQLite3Table table_bkg = g_db.getTable("select id,name,data,data_len,x,y,width,height,filename from agv_bkg;");
        if (table_bkg.numRows() > 0 && table_bkg.numFields() != 9)return false;
        for (int row = 0; row < table_bkg.numRows(); row++)
        {
            table_bkg.setRow(row);

            int id = atoi(table_bkg.fieldValue(0));
            std::string name = std::string(table_bkg.fieldValue(1));

            CppSQLite3Binary blob;
            blob.setEncoded((unsigned char*)table_bkg.fieldValue(2));
            unsigned char *data = new unsigned char[blob.getBinaryLength()];
            memcpy(data, blob.getBinary(), blob.getBinaryLength());
            int data_len = atoi(table_bkg.fieldValue(3));
            int x = atoi(table_bkg.fieldValue(4));
            int y = atoi(table_bkg.fieldValue(5));
            int width = atoi(table_bkg.fieldValue(6));
            int height = atoi(table_bkg.fieldValue(7));
            std::string filename = std::string(table_bkg.fieldValue(8));
            MapBackground *bkg = new MapBackground(id, name, (char *)data, data_len, width, height, filename);
            bkg->setX(x);
            bkg->setY(y);
            g_onemap.addSpirit(bkg);
        }


        CppSQLite3Table table_floor = g_db.getTable("select id,name,point,path,bkg,originX,originY,rate from agv_floor;");
        if (table_floor.numRows() > 0 && table_floor.numFields() != 8)return false;
        for (int row = 0; row < table_floor.numRows(); row++)
        {
            table_floor.setRow(row);

            int id = atoi(table_floor.fieldValue(0));
            std::string name = std::string(table_floor.fieldValue(1));

            MapFloor *mfloor = new MapFloor(id, name);

            std::string pointstr = std::string(table_floor.fieldValue(2));
            std::string pathstr = std::string(table_floor.fieldValue(3));
            int bkg = atoi(table_floor.fieldValue(4));
            int originX = atoi(table_floor.fieldValue(5));
            int originY = atoi(table_floor.fieldValue(6));
            double rate = atof(table_floor.fieldValue(7));

            std::vector<std::string> pvs = split(pointstr, ";");
            for (auto p : pvs) {
                int intp;
                std::stringstream ss;
                ss << p;
                ss >> intp;
                mfloor->addPoint(intp);
            }

            std::vector<std::string> avs = split(pathstr, ";");
            for (auto p : avs) {
                int intp;
                std::stringstream ss;
                ss << p;
                ss >> intp;
                mfloor->addPath(intp);
            }

            mfloor->setBkg(bkg);
            mfloor->setOriginX(originX);
            mfloor->setOriginY(originY);
            mfloor->setRate(rate);
            g_onemap.addSpirit(mfloor);
        }


        CppSQLite3Table table_block = g_db.getTable("select id,name,spirits from agv_block;");
        if (table_block.numRows() > 0 && table_block.numFields() != 3)return false;
        for (int row = 0; row < table_block.numRows(); row++)
        {
            table_block.setRow(row);

            int id = atoi(table_block.fieldValue(0));
            std::string name = std::string(table_block.fieldValue(1));

            MapBlock *mblock = new MapBlock(id, name);

            std::string pointstr = std::string(table_block.fieldValue(2));
            std::vector<std::string> pvs = split(pointstr, ";");
            for (auto p : pvs) {
                int intp;
                std::stringstream ss;
                ss << p;
                ss >> intp;
                mblock->addSpirit(intp);
            }

            g_onemap.addSpirit(mblock);
        }

        CppSQLite3Table table_group = g_db.getTable("select id,name,spirits,groupType from agv_group;");
        if (table_group.numRows() > 0 && table_group.numFields() != 4)return false;
        for (int row = 0; row < table_group.numRows(); row++)
        {
            table_group.setRow(row);

            int id = atoi(table_group.fieldValue(0));
            std::string name = std::string(table_group.fieldValue(1));
            int groupType = atoi(table_group.fieldValue(3));

            MapGroup *mgroup = new MapGroup(id, name, groupType);

            std::string pointstr = std::string(table_group.fieldValue(2));

            std::vector<std::string> pvs = split(pointstr, ";");
            for (auto p : pvs) {
                int intp;
                std::stringstream ss;
                ss << p;
                ss >> intp;
                mgroup->addSpirit(intp);
            }

            g_onemap.addSpirit(mgroup);
        }

        int max_id = 0;
        auto ps = g_onemap.getAllElement();
        for (auto p : ps) {
            if (p->getId() > max_id)max_id = p->getId();
        }
        g_onemap.setMaxId(max_id);
        getReverseLines();
        getAdj();
        check();
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        return false;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}", e.what());
        return false;
    }
    return true;
}

//std::vector<int> MapManager::getBestPathDy(int agv, int lastStation, int startStation, int endStation, int &distance, bool changeDirect)
//{
//    //获取路径的必经点
//    std::queue<int> chd_station;
//    int startBlock = getBlock(startStation);
//    int endBlock = getBlock(endStation);
//    if(m_chd_station.find(std::make_pair(startBlock, endBlock)) != m_chd_station.end() )
//    {
//        chd_station = m_chd_station[std::make_pair(startBlock, endBlock)];
//    }
//    //判断是否有必经点
//    if(chd_station.size())
//    {
//        chd_station.push(endStation);
//        std::vector<int> exec_path;
//        distance = 0;
//        do
//        {
//            int pos = chd_station.front();
//            int sub_dis;
//            std::vector<int> sub_path = getBestPath(agv, lastStation, startStation, pos, sub_dis, changeDirect);
//            if(!sub_path.size())
//            {
//                return sub_path;
//            }
//            else
//            {
//                distance += sub_dis;
//                exec_path.insert(exec_path.end(), sub_path.begin(), sub_path.end());
//                startStation = pos;
//                auto ptr = g_onemap.getSpiritById(exec_path.back());
//                if(MapSpirit::Map_Sprite_Type_Path != ptr->getSpiritType())
//                {
//                    exec_path.clear();
//                    return exec_path;
//                }
//                MapPath *lastPath = static_cast<MapPath *>(ptr);
//                lastStation = lastPath->getEnd();
//                chd_station.pop();
//            }
//        }while(chd_station.size());
//        return exec_path;
//    }
//    else
//    {
//        return getBestPath(agv, lastStation, startStation, endStation, distance, changeDirect);
//    }
//}

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

bool MapManager::pathPassable(MapPath *line, int agvId, std::vector<int> passable_uturnPoints) {

    auto pend = getPointById(line->getEnd());
    if(pend == nullptr)return false;

    if(std::find(passable_uturnPoints.begin(),passable_uturnPoints.end(), line->getStart()) == passable_uturnPoints.end() &&
            std::find(passable_uturnPoints.begin(), passable_uturnPoints.end(), line->getEnd()) == passable_uturnPoints.end())
    {
        //忽略起点和终点直连装卸货点及不可调头的判断
        if(pend->getPointType() == MapPoint::Map_Point_Type_CHARGE ||
                pend->getPointType() == MapPoint::Map_Point_Type_LOAD ||
                pend->getPointType() == MapPoint::Map_Point_Type_UNLOAD ||
                pend->getPointType() == MapPoint::Map_Point_Type_NO_UTURN ||
                pend->getPointType() == MapPoint::Map_Point_Type_LOAD_UNLOAD)return false;
    }

    //判断反向线路占用
    auto reverses = m_reverseLines[line->getId()];
    for(auto reverse:reverses){
        if (reverse > 0) {
            if (line_occuagvs[reverse].size() > 1 || (line_occuagvs[reverse].size() == 1 && *(line_occuagvs[reverse].begin()) != agvId))
                return false;
        }
    }

    //判断终点占用
    if (station_occuagv[line->getEnd()] != 0 && station_occuagv[line->getEnd()] != agvId)
        return false;


    //判断group占用
    auto groups = g_onemap.getGroups(COMMON_GROUP);
    UNIQUE_LCK(groupMtx);
    for (auto group : groups) {
        auto sps = group->getSpirits();
        if (std::find(sps.begin(), sps.end(), line->getId()) != sps.end() ||
                std::find(sps.begin(), sps.end(), line->getEnd()) != sps.end())
        {
            //该线路/线路终点属于这个block
            //判断该block是否有agv以外的其他agv
            if (group_occuagv.find(group->getId()) != group_occuagv.end() && group_occuagv[group->getId()].first != agvId)
                return false;
        }
    }

    return true;
}

bool MapManager::pathPassable(MapPath *line, int agvId) {
    //TODO: to delete
    if (line_occuagvs[line->getId()].size() > 1 || (line_occuagvs[line->getId()].size() == 1 && *(line_occuagvs[line->getId()].begin()) != agvId))
        return false;

    auto pend = getPointById(line->getEnd());
    if(pend == nullptr)return false;

    if(pend->getPointType() == MapPoint::Map_Point_Type_CHARGE ||
            pend->getPointType() == MapPoint::Map_Point_Type_LOAD ||
            pend->getPointType() == MapPoint::Map_Point_Type_UNLOAD ||
            pend->getPointType() == MapPoint::Map_Point_Type_LOAD_UNLOAD||
            pend->getPointType() == MapPoint::Map_Point_Type_NO_UTURN)return false;


    //判断反向线路占用
    auto reverses = m_reverseLines[line->getId()];
    for(auto reverse:reverses){
        if (reverse > 0) {
            if (line_occuagvs[reverse].size() > 1 || (line_occuagvs[reverse].size() == 1 && *(line_occuagvs[reverse].begin()) != agvId))
                return false;
        }
    }

    //判断终点占用
    if (station_occuagv[line->getEnd()] != 0 && station_occuagv[line->getEnd()] != agvId)
        return false;

    //判断group占用
    auto groups = g_onemap.getGroups(COMMON_GROUP);
    UNIQUE_LCK(groupMtx);
    for (auto group : groups) {
        auto sps = group->getSpirits();
        if (std::find(sps.begin(), sps.end(), line->getId()) != sps.end() ||
                std::find(sps.begin(), sps.end(), line->getEnd()) != sps.end())
        {
            //该线路/线路终点属于这个group
            //判断该group是否有agv以外的其他agv
            if (group_occuagv.find(group->getId()) != group_occuagv.end() && group_occuagv[group->getId()].first != agvId)
                return false;
        }
    }

    return true;
}

//获取 距离 目标点位 最近的躲避点//不计算可行性
int MapManager::getNearestHaltStation(int agvId, int aimStation)
{
    int halt = -1;
    int minDistance = DISTANCE_INFINITY;
    auto ae = g_onemap.getAllElement();
    for(auto e:ae){
        //MapSpirit ee;
        if(e->getSpiritType() == MapSpirit::Map_Sprite_Type_Point){
            if((static_cast<MapPoint *>(e))->getPointType() == MapPoint::Map_Point_Type_HALT)
            {
                //这是个避让点
                //如果这个躲避点已经被占用？
                if (station_occuagv[e->getId()] != 0 && station_occuagv[e->getId()] != agvId)
                    continue;

                //判断group占用
                auto groups = g_onemap.getGroups(HALT_GROUP);
                for (auto group : groups)
                {
                    auto sps = group->getSpirits();
                    if (std::find(sps.begin(), sps.end(), e->getId()) != sps.end())
                    {
                        //该线路/线路终点属于这个block
                        //判断该block是否有agv以外的其他agv
                        UNIQUE_LCK(groupMtx);
                        if (group_occuagv.find(group->getId()) != group_occuagv.end() && group_occuagv[group->getId()].first != agvId)
                            continue;
                    }
                }

                //计算躲避点到目标点的最近距离
                int distance = DISTANCE_INFINITY;
                getPath(e->getId(),aimStation,distance,false);
                if(distance<minDistance)
                {
                    minDistance = distance;
                    halt = e->getId();
                }
            }
        }
    }

    return halt;
}

std::vector<int> MapManager::getPath(int from, int to, int &distance, bool changeDirect)
{
    std::vector<int> result;

    //判断station是否正确
    if (from <= 0 || to <=0) return result;
    if(from == to)
    {
        distance = 0;
        return result;
    }

    auto startStationPtr = g_onemap.getSpiritById(from);
    auto endStationPtr = g_onemap.getSpiritById(to);

    if (startStationPtr == nullptr || endStationPtr == nullptr)return result;

    if (startStationPtr->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
            || endStationPtr->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        return result;

    std::multimap<int, int> Q;// distance -- lineid
    auto paths = g_onemap.getPaths();

    struct LineDijkInfo {
        int father = 0;
        int distance = DISTANCE_INFINITY;
        int color = AGV_LINE_COLOR_WHITE;
    };
    std::map<int, LineDijkInfo> lineDistanceColors;

    for (auto path : paths) {
        lineDistanceColors[path->getId()].father = 0;
        lineDistanceColors[path->getId()].distance = DISTANCE_INFINITY;
        lineDistanceColors[path->getId()].color = AGV_LINE_COLOR_WHITE;
    }

    for (auto line : paths) {
        if (line->getStart() == from) {
            lineDistanceColors[line->getId()].distance = line->getLength();
            lineDistanceColors[line->getId()].color = AGV_LINE_COLOR_GRAY;
            Q.insert(std::make_pair(lineDistanceColors[line->getId()].distance, line->getId()));
        }
    }

    while (Q.size() > 0) {
        auto front = Q.begin();
        int startLine = front->second;

        std::vector<int> adjs = m_adj[startLine];
        for (auto adj : adjs)
        {
            if (lineDistanceColors[adj].color == AGV_LINE_COLOR_BLACK)continue;

            MapSpirit *pp = g_onemap.getSpiritById(adj);
            if (pp->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)continue;
            MapPath *path = static_cast<MapPath *>(pp);
            if (lineDistanceColors[adj].color == AGV_LINE_COLOR_WHITE) {
                lineDistanceColors[adj].distance = lineDistanceColors[startLine].distance + path->getLength();
                lineDistanceColors[adj].color = AGV_LINE_COLOR_GRAY;
                lineDistanceColors[adj].father = startLine;
                Q.insert(std::make_pair(lineDistanceColors[adj].distance, adj));
            }
            else if (lineDistanceColors[adj].color == AGV_LINE_COLOR_GRAY) {
                if (lineDistanceColors[adj].distance > lineDistanceColors[startLine].distance + path->getLength()) {
                    lineDistanceColors[adj].distance = lineDistanceColors[startLine].distance + path->getLength();
                    lineDistanceColors[adj].father = startLine;

                    //更新Q中的 adj

                    //删除旧的
                    for (auto iiitr = Q.begin(); iiitr != Q.end();) {
                        if (iiitr->second == adj) {
                            iiitr = Q.erase(iiitr);
                        }
                        else
                            iiitr++;
                    }
                    //加入新的
                    Q.insert(std::make_pair(lineDistanceColors[adj].distance, adj));
                }
            }
        }
        lineDistanceColors[startLine].color = AGV_LINE_COLOR_BLACK;
        //erase startLine
        for (auto itr = Q.begin(); itr != Q.end();) {
            if (itr->second == startLine) {
                itr = Q.erase(itr);
            }
            else
                itr++;
        }
    }

    int index = 0;
    int minDis = DISTANCE_INFINITY;

    for (auto ll : paths) {
        if (ll->getEnd() == to) {
            if (lineDistanceColors[ll->getId()].distance < minDis) {
                minDis = lineDistanceColors[ll->getId()].distance;
                index = ll->getId();
            }
        }
    }

    distance = minDis;

    while (true) {
        if (index == 0)break;
        result.push_back(index);
        index = lineDistanceColors[index].father;
    }
    std::reverse(result.begin(), result.end());

    return result;
}


std::vector<int> MapManager::getPath(int agv, int lastStation, int startStation, int endStation, int &distance, bool changeDirect)
{
    std::vector<int> result;
    distance = DISTANCE_INFINITY;

    auto paths = g_onemap.getPaths();

    //判断station是否正确
    if(lastStation <=0 && startStation <=0 )return result;
    if (endStation <= 0)return result;
    if (lastStation <= 0) lastStation = startStation;
    if (startStation <= 0) startStation = lastStation;

    auto lastStationPtr = g_onemap.getSpiritById(lastStation);
    auto startStationPtr = g_onemap.getSpiritById(startStation);
    auto endStationPtr = g_onemap.getSpiritById(endStation);

    if (lastStationPtr == nullptr || startStationPtr == nullptr || endStationPtr == nullptr)return result;


    if (lastStationPtr->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
            || startStationPtr->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
            || endStationPtr->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        return result;


    //判断站点占用清空
    if (station_occuagv[startStation] != 0 && station_occuagv[startStation] != agv)return result;
    if (station_occuagv[endStation] != 0 && station_occuagv[endStation] != agv)return result;

    if (startStation == endStation) {
        distance = 0;
        if (changeDirect && lastStation != startStation) {
            for (auto path : paths) {
                if (path->getStart() == lastStation && path->getEnd() == startStation) {
                    result.push_back(path->getId());
                    distance = path->getLength();
                }
            }
        }
        return result;
    }

    std::multimap<int, int> Q;// distance -- lineid

    struct LineDijkInfo {
        int father = 0;
        int distance = DISTANCE_INFINITY;
        int color = AGV_LINE_COLOR_WHITE;
    };
    std::map<int, LineDijkInfo> lineDistanceColors;

    std::vector<int> passable_uturnPoints;
    passable_uturnPoints.push_back(startStation);
    passable_uturnPoints.push_back(endStation);

    //初始化，所有线路 距离为无限远、color为尚未标记
    for (auto path : paths) {
        lineDistanceColors[path->getId()].father = 0;
        lineDistanceColors[path->getId()].distance = DISTANCE_INFINITY;
        lineDistanceColors[path->getId()].color = AGV_LINE_COLOR_WHITE;

        if(path->getStart() == startStation)
        {
            auto pend = getPointById(path->getEnd());
            if(pend == nullptr)continue;
            if(MapPoint::Map_Point_Type_NO_UTURN == pend->getPointType() || MapPoint::Map_Point_Type_LOAD == pend->getPointType()
                    || MapPoint::Map_Point_Type_LOAD == pend->getPointType() || MapPoint::Map_Point_Type_LOAD_UNLOAD == pend->getPointType())
            {
                passable_uturnPoints.push_back(pend->getId());
            }
        }
        else if(path->getEnd() == endStation)
        {
            auto pstart = getPointById(path->getStart());
            if(pstart == nullptr)continue;
            if(MapPoint::Map_Point_Type_NO_UTURN == pstart->getPointType()|| MapPoint::Map_Point_Type_LOAD == pstart->getPointType()
                    || MapPoint::Map_Point_Type_LOAD == pstart->getPointType() || MapPoint::Map_Point_Type_LOAD_UNLOAD == pstart->getPointType())
            {
                passable_uturnPoints.push_back(pstart->getId());
            }
        }
    }


    ////增加一种通行的判定：
    ////如果AGV2 要从 C点 到达 D点，同事路过B点。
    ////同时AGV1 要从 A点 到达 B点。如果AGV1先到达B点，会导致AGV2 无法继续运行。
    ////判定终点的线路 是否占用
    ////endPoint是终点
    {
        for (auto templine : paths) {
            if (templine->getStart() == endStation) {
                if (line_occuagvs[templine->getId()].size() > 1 || (line_occuagvs[templine->getId()].size() == 1 && *(line_occuagvs[templine->getId()].begin()) != agv)) {
                    //TODO:该方式到达这个地方 不可行.该线路 置黑、
                    lineDistanceColors[templine->getId()].color = AGV_LINE_COLOR_BLACK;
                    lineDistanceColors[templine->getId()].distance = DISTANCE_INFINITY;
                }
            }
        }
    }

    if (lastStation == startStation) {
        for (auto line : paths) {
            if (line->getStart() == lastStation) {
                if (pathPassable(line, agv,passable_uturnPoints)) {
                    if (lineDistanceColors[line->getId()].color == AGV_LINE_COLOR_BLACK)continue;
                    lineDistanceColors[line->getId()].distance = line->getLength();
                    lineDistanceColors[line->getId()].color = AGV_LINE_COLOR_GRAY;
                    Q.insert(std::make_pair(lineDistanceColors[line->getId()].distance, line->getId()));
                }
            }
        }
    }
    else {
        for (auto line : paths) {
            if (line->getStart() == startStation) {
                //            if (line->getStart() == startStation && line->getEnd() != lastStation) {
                if (pathPassable(line, agv,passable_uturnPoints)) {
                    if (lineDistanceColors[line->getId()].color == AGV_LINE_COLOR_BLACK)continue;
                    lineDistanceColors[line->getId()].distance = line->getLength();
                    lineDistanceColors[line->getId()].color = AGV_LINE_COLOR_GRAY;
                    Q.insert(std::make_pair(lineDistanceColors[line->getId()].distance, line->getId()));
                }
            }
        }
    }

    while (Q.size() > 0) {
        auto front = Q.begin();
        int startLine = front->second;

        std::vector<int> adjs = m_adj[startLine];
        for (auto adj : adjs)
        {
            if (lineDistanceColors[adj].color == AGV_LINE_COLOR_BLACK)continue;

            MapPath *path = g_onemap.getPathById(adj);
            if (path == nullptr)continue;
            if (!pathPassable(path, agv,passable_uturnPoints))continue;
            if (lineDistanceColors[adj].color == AGV_LINE_COLOR_WHITE) {
                lineDistanceColors[adj].distance = lineDistanceColors[startLine].distance + path->getLength();
                lineDistanceColors[adj].color = AGV_LINE_COLOR_GRAY;
                lineDistanceColors[adj].father = startLine;
                Q.insert(std::make_pair(lineDistanceColors[adj].distance, adj));
            }
            else if (lineDistanceColors[adj].color == AGV_LINE_COLOR_GRAY) {
                if (lineDistanceColors[adj].distance > lineDistanceColors[startLine].distance + path->getLength()) {
                    //更新father和距离
                    lineDistanceColors[adj].distance = lineDistanceColors[startLine].distance + path->getLength();
                    lineDistanceColors[adj].father = startLine;

                    //更新Q中的 adj

                    //删除旧的
                    for (auto iiitr = Q.begin(); iiitr != Q.end();) {
                        if (iiitr->second == adj) {
                            iiitr = Q.erase(iiitr);
                        }
                        else
                            iiitr++;
                    }
                    //加入新的
                    Q.insert(std::make_pair(lineDistanceColors[adj].distance, adj));
                }
            }
        }
        lineDistanceColors[startLine].color = AGV_LINE_COLOR_BLACK;
        //erase startLine
        for (auto itr = Q.begin(); itr != Q.end();) {
            if (itr->second == startLine) {
                itr = Q.erase(itr);
            }
            else
                itr++;
        }
    }

    int index = 0;
    int minDis = DISTANCE_INFINITY;

    for (auto ll : paths) {
        if (ll->getEnd() == endStation) {
            if (lineDistanceColors[ll->getId()].distance < minDis) {
                minDis = lineDistanceColors[ll->getId()].distance;
                index = ll->getId();
            }
        }
    }

    distance = minDis;

    while (true) {
        if (index == 0)break;
        result.push_back(index);
        index = lineDistanceColors[index].father;
    }
    std::reverse(result.begin(), result.end());

    if (result.size() > 0 && lastStation != startStation) {
        if (!changeDirect) {
            int  agv_line = *(result.begin());
            MapSpirit *sp = g_onemap.getSpiritById(agv_line);
            if (sp->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
                MapPath *path = static_cast<MapPath *>(sp);
                if (path->getStart() == lastStation && path->getEnd() == startStation) {
                    result.erase(result.begin());
                }
            }
        }
    }

    return result;
}


void MapManager::getReverseLines()
{
    std::list<MapPath *> paths = g_onemap.getPaths();

    //TODO:对于临近的 a ->  b 和  b' -> a'进行反向判断
    for (auto a : paths) {
        for (auto b : paths) {
            if (a == b)continue;

            int aEndId = a->getEnd();
            int aStartId = a->getStart();
            int bStartId = b->getStart();
            int bEndId = b->getEnd();
            MapSpirit *aEnd =  getMapSpiritById(aEndId);
            MapSpirit *bStart =  getMapSpiritById(bStartId);
            MapSpirit *bEnd =  getMapSpiritById(bEndId);
            MapSpirit *aStart =  getMapSpiritById(aStartId);

            if(aEnd == nullptr || aEnd->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
                    ||bEnd == nullptr || bEnd->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
                    ||bStart == nullptr || bStart->getSpiritType() != MapSpirit::Map_Sprite_Type_Point
                    ||aStart == nullptr || aStart->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)continue;

            MapPoint *pAEnd = static_cast<MapPoint *>(aEnd);
            MapPoint *pBStart = static_cast<MapPoint *>(bStart);
            MapPoint *pBEnd = static_cast<MapPoint *>(bEnd);
            MapPoint *pAStart = static_cast<MapPoint *>(aStart);

            auto lists = m_reverseLines[a->getId()];

            if (a->getEnd() == b->getStart() && a->getStart() == b->getEnd()) {
                lists.push_back(b->getId());
                m_reverseLines[a->getId()] = lists;
            }else if(pAEnd->getRealX() == pBStart->getRealX() && pAEnd->getRealY() == pBStart->getRealY()
                     && pBEnd->getRealX() == pAStart->getRealX() && pBEnd->getRealY() == pAStart->getRealY()){
                lists.push_back(b->getId());
                m_reverseLines[a->getId()] = lists;
            }
        }
    }
    return ;
}

//check if exists all element
void MapManager::check()
{
    //check lines!
    bool changed = false;
    auto paths = g_onemap.getPaths();
    for(auto path:paths){
        auto start = path->getStart();
        auto end = path->getEnd();
        if(g_onemap.getPointById(start) == nullptr
                ||g_onemap.getPointById(end) == nullptr){
            g_onemap.removeSpiritById(path->getId());
            changed = true;
        }
    }
    if(changed){
        save();
        changed = false;
    }

    //check floor!
    auto floors = g_onemap.getFloors();
    for(auto floor:floors){
        auto points = floor->getPoints();
        for(auto p:points){
            if(g_onemap.getPointById(p) == nullptr){
                floor->removePoint(p);
                changed = true;
            }
        }
        auto pathss = floor->getPaths();
        for(auto pp:pathss){
            if(g_onemap.getPathById(pp) == nullptr){
                floor->removePath(pp);
                changed = true;
            }
        }
    }
    if(changed){
        save();
        changed = false;
    }

    //check group!
    auto groups = g_onemap.getGroups();
    for(auto group:groups){
        auto ps = group->getSpirits();
        for(auto p:ps){
            if(g_onemap.getPointById(p)==nullptr
                    &&g_onemap.getPathById(p) == nullptr){
                group->removeSpirit(p);
                changed = true;
            }
        }
    }
    if(changed){
        save();
        changed = false;
    }

    //check block!
    auto blocks = g_onemap.getBlocks();
    for(auto block:blocks){
        auto ps = block->getSpirits();
        for(auto p:ps){
            if(g_onemap.getPointById(p)==nullptr
                    &&g_onemap.getPathById(p) == nullptr){
                block->removeSpirit(p);
                changed = true;
            }
        }
    }
    if(changed){
        save();
        changed = false;
    }
}

void MapManager::getAdj()
{
    std::list<MapPath *> paths = g_onemap.getPaths();

    for (auto a : paths) {
        for (auto b : paths) {
            if (a == b)continue;
            if (a->getEnd() == b->getStart() && a->getStart() != b->getEnd()) {
                m_adj[a->getId()].push_back(b->getId());
            }
        }
    }

    return ;
}

void MapManager::clear()
{
    line_occuagvs.clear();
    station_occuagv.clear();
    BlockManager::getInstance()->clear();
    m_reverseLines.clear();
    m_adj.clear();
    g_onemap.clear();
}

bool MapManager::isSameFloor(int floor, int station)
{
    std::vector<int> stations = getStations(floor);
    int count = std::count(stations.begin(), stations.end(), station);
    return count > 0 ? true : false;
}

int MapManager::getFloor(int spiritID)
{
    int floor = -1;
    std::list<MapFloor *> floors = g_onemap.getFloors();
    for(auto onefloor:floors)
    {
        std::list<int> pointlist = onefloor->getPoints();

        if(std::find(pointlist.begin(), pointlist.end(), spiritID) != pointlist.end())
        {
            floor = onefloor->getId(); //std::stoi(onefloor->getName().substr(6));
            break;
        }

        std::list<int> pathlist = onefloor->getPaths();

        if (std::find(pathlist.begin(), pathlist.end(), spiritID) != pathlist.end())
        {
            floor = onefloor->getId(); //std::stoi(onefloor->getName().substr(6));
            break;
        }
    }
    return floor;
}

std::vector<int> MapManager::getGroup(int spiritID)
{
    std::vector<int> group;
    auto groups = g_onemap.getGroups(COMMON_GROUP);
    for (auto onegroup : groups)
    {
        std::list<int> spiritslist = onegroup->getSpirits();

        if (std::find(spiritslist.begin(), spiritslist.end(), spiritID) != spiritslist.end())
        {
            group.push_back(onegroup->getId());
        }
    }
    return group;
}

std::vector<int> MapManager::getBlocks(int spiritID)
{
    std::vector<int> blocks;
    std::list<MapBlock *> bs = g_onemap.getBlocks();
    for(auto oneblock:bs)
    {
        std::list<int> spiritslist = oneblock->getSpirits();

        if(std::find(spiritslist.begin(), spiritslist.end(), spiritID) != spiritslist.end())
        {
            blocks.push_back(oneblock->getId());
        }
    }
    return blocks;
}

std::list<int> MapManager::getOccuSpirit(int agvId)
{
    std::list<int> l;
    for (auto s : station_occuagv) {
        if (s.second == agvId) {
            l.push_back(s.first);
        }
    }

    for (auto s : line_occuagvs) {
        if(std::find(s.second.begin(),s.second.end(),agvId)!=s.second.end())
        {
            l.push_back(s.first);
        }
    }
    return l;
}

//bool MapManager::blockPassable(int blockId,int agvId)
//{
//    UNIQUE_LCK(blockMtx);
//    //该线路/线路终点属于这个block
//    //判断该block是否有agv以外的其他agv
//    if (block_occuagv.find(blockId) != block_occuagv.end() && block_occuagv[blockId].first != agvId)
//        return false;
//    return true;
//}

std::vector<int> MapManager::getStations(int floor)
{
    std::vector<int> allstation;
    if(floor == 0)
    {
        allstation = g_onemap.getStations();
    }
    else
    {
        std::string floorname;
        floorname.append("floor_").append(std::to_string(floor));
        std::list<MapFloor *> floors = g_onemap.getFloors();
        for(auto currentfloor:floors)
        {
            if(currentfloor->getName() == floorname)
            {
                std::list<int> pointlist = currentfloor->getPoints();
                for(auto point:pointlist)
                {
                    allstation.push_back(point);
                }
                break;
            }
        }
    }
    return allstation;
}

void MapManager::interSetMap(SessionPtr conn, const Json::Value &request)
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
            g_db.execDML("delete from agv_group");

            //TODO:
            //1.解析站点
            for (int i = 0; i < request["points"].size(); ++i)
            {
                Json::Value station = request["points"][i];
                int id = station["id"].asInt();
                std::string name = station["name"].asString();
                int station_type = station["type"].asInt();
                int x = station["x"].asInt();
                int y = station["y"].asInt();
                int realX = station["realX"].asInt();
                int realY = station["realY"].asInt();
                int realA = station["realA"].asInt();
                int labelXoffset = station["labelXoffset"].asInt();
                int labelYoffset = station["labelYoffset"].asInt();
                bool mapchange = station["mapchange"].asBool();
                bool locked = station["locked"].asBool();
                std::string ip = station["ip"].asString();
                int port = station["port"].asInt();
                int agvType = station["agvType"].asInt();
                std::string lineId = station["lineId"].asString();
                MapPoint::Map_Point_Type t = static_cast<MapPoint::Map_Point_Type>(station_type);
                MapPoint *p = new MapPoint(id, name, t, x, y, realX, realY, realA, labelXoffset, labelYoffset, mapchange, locked,ip,port,agvType,lineId);
                g_onemap.addSpirit(p);
            }

            //2.解析线路
            for (int i = 0; i < request["paths"].size(); ++i)
            {
                Json::Value line = request["paths"][i];
                int id = line["id"].asInt();
                std::string name = line["name"].asString();
                int type = line["type"].asInt();
                int start = line["start"].asInt();
                int end = line["end"].asInt();
                int p1x = line["p1x"].asInt();
                int p1y = line["p1y"].asInt();
                int p2x = line["p2x"].asInt();
                int p2y = line["p2y"].asInt();
                int length = line["length"].asInt();
                bool locked = line["locked"].asBool();
                double speed = line["speed"].asDouble();

                MapPath::Map_Path_Type t = static_cast<MapPath::Map_Path_Type>(type);
                MapPath *p = new MapPath(id, name, start, end, t, length, p1x, p1y, p2x, p2y, locked,speed);
                g_onemap.addSpirit(p);
            }

            //4.解析背景图片
            for (int i = 0; i < request["bkgs"].size(); ++i)
            {
                Json::Value bkg = request["bkgs"][i];
                int id = bkg["id"].asInt();
                std::string name = bkg["name"].asString();
                std::string database64 = bkg["data"].asString();
                //combined_logger->debug("database64 length={0},\n left10={1},\n right10={2}", database64.length(), database64.substr(0, 10).c_str(), database64.substr(database64.length() - 10).c_str());

                int lenlen = Base64decode_len(database64.c_str());
                char *data = new char[lenlen];
                memset(data, 0, lenlen);
                lenlen = Base64decode(data, database64.c_str());

                ////TODO:输出16进制
                //for (int i = 0; i < 10; ++i) {
                //	printf(" %02X", data[i] & 0xFF);
                //}
                //printf("\n");

                //for (int i = 0; i < 10; ++i) {
                //	printf(" %02X", data[lenlen-10+i] & 0xFF);
                //}
                //printf("\n");


                int imgdatalen = bkg["data_len"].asInt();
                int width = bkg["width"].asInt();
                int height = bkg["height"].asInt();
                int x = bkg["x"].asInt();
                int y = bkg["y"].asInt();
                std::string filename = bkg["filename"].asString();
                MapBackground *p = new MapBackground(id, name, data, lenlen, width, height, filename);
                p->setX(x);
                p->setY(y);
                g_onemap.addSpirit(p);
            }

            //3.解析楼层
            for (int i = 0; i < request["floors"].size(); ++i)
            {
                Json::Value floor = request["floors"][i];
                int id = floor["id"].asInt();
                std::string name = floor["name"].asString();
                Json::Value points = floor["points"];
                Json::Value paths = floor["paths"];
                int bkg = floor["bkg"].asInt();
                int originX = floor["originX"].asInt();
                int originY = floor["originY"].asInt();
                double rate = floor["rate"].asDouble();
                MapFloor *p = new MapFloor(id, name);
                p->setBkg(bkg);
                p->setOriginX(originX);
                p->setOriginY(originY);
                p->setRate(rate);
                for (int k = 0; k < points.size(); ++k) {
                    Json::Value point = points[k];
                    p->addPoint(point.asInt());
                }
                for (int k = 0; k < paths.size(); ++k) {
                    Json::Value path = paths[k];
                    p->addPath(path.asInt());
                }
                g_onemap.addSpirit(p);
            }

            //5.解析block
            for (unsigned int i = 0; i < request["blocks"].size(); ++i)
            {
                Json::Value block = request["blocks"][i];
                int id = block["id"].asInt();
                std::string name = block["name"].asString();
                Json::Value spirits = block["spirits"];
                MapBlock *p = new MapBlock(id, name);
                for (unsigned int k = 0; k < spirits.size(); ++k) {
                    Json::Value spirit = spirits[k];
                    p->addSpirit(spirit.asInt());
                }
                g_onemap.addSpirit(p);
            }

            //6.解析group
            for (unsigned int i = 0; i < request["groups"].size(); ++i)
            {
                Json::Value group = request["groups"][i];
                int id = group["id"].asInt();
                std::string name = group["name"].asString();
                int groupType = group["type"].asInt();
                Json::Value spirits = group["spirits"];
                MapGroup *p = new MapGroup(id, name, groupType);
                for (unsigned int k = 0; k < spirits.size(); ++k) {
                    Json::Value spirit = spirits[k];
                    p->addSpirit(spirit.asInt());
                }

                g_onemap.addSpirit(p);
            }

            int max_id = request["maxId"].asInt();
            g_onemap.setMaxId(max_id);


            //构建反向线路和adj
            getReverseLines();
            getAdj();

            if (!save()) {
                response["result"] = RETURN_MSG_RESULT_FAIL;
                response["error_code"] = RETURN_MSG_ERROR_CODE_SAVE_SQL_FAIL;
                clear();
            }
        }
        catch (CppSQLite3Exception e) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
            response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        }
        catch (std::exception e) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            std::stringstream ss;
            ss << "info:" << e.what();
            response["error_info"] = ss.str();
            combined_logger->error("sqlerr code:{0}", e.what());
        }
    }

    mapModifying = false;

    conn->send(response);
}

void MapManager::interGetMap(SessionPtr conn, const Json::Value &request)
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

        std::list<MapSpirit *> allspirit = g_onemap.getAllElement();

        Json::Value v_points;
        Json::Value v_paths;
        Json::Value v_floors;
        Json::Value v_bkgs;
        Json::Value v_blocks;
        Json::Value v_groups;
        for (auto spirit : allspirit)
        {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point) {
                MapPoint *p = static_cast<MapPoint *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();
                pv["type"] = p->getPointType();
                pv["x"] = p->getX();
                pv["y"] = p->getY();
                pv["realX"] = p->getRealX();
                pv["realY"] = p->getRealY();
                pv["realA"] = p->getRealA();
                pv["labelXoffset"] = p->getLabelXoffset();
                pv["labelYoffset"] = p->getLabelYoffset();
                pv["mapchange"] = p->getMapChange();
                pv["locked"] = p->getLocked();
                pv["ip"] = p->getIp();
                pv["port"] = p->getPort();
                pv["agvType"] = p->getAgvType();
                pv["lineId"] = p->getLineId();
                v_points.append(pv);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
                MapPath *p = static_cast<MapPath *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();
                pv["type"] = p->getPathType();
                pv["start"] = p->getStart();
                pv["end"] = p->getEnd();
                pv["p1x"] = p->getP1x();
                pv["p1y"] = p->getP1y();
                pv["p2x"] = p->getP2x();
                pv["p2y"] = p->getP2y();
                pv["length"] = p->getLength();
                pv["locked"] = p->getLocked();
                pv["speed"] = p->getSpeed();
                v_paths.append(pv);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Background) {
                MapBackground *p = static_cast<MapBackground *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();
                int lenlen = Base64encode_len(p->getImgDataLen());
                char *ss = new char[lenlen];
                Base64encode(ss, p->getImgData(), p->getImgDataLen());
                pv["data"] = std::string(ss, lenlen);
                delete[] ss;
                pv["data_len"] = p->getImgDataLen();
                pv["width"] = p->getWidth();
                pv["height"] = p->getHeight();
                pv["x"] = p->getX();
                pv["y"] = p->getY();
                pv["filename"] = p->getFileName();
                v_bkgs.append(pv);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Floor) {
                MapFloor *p = static_cast<MapFloor *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();
                pv["bkg"] = p->getBkg();
                pv["originX"] = p->getOriginX();
                pv["originY"] = p->getOriginY();
                pv["rate"] = p->getRate();
                Json::Value ppv;
                auto points = p->getPoints();
                int kk = 0;
                for (auto p : points) {
                    ppv[kk++] = p;
                }
                if (!ppv.isNull() > 0)
                    pv["points"] = ppv;

                Json::Value ppv2;
                auto paths = p->getPaths();
                int kk2 = 0;
                for (auto p : paths) {
                    ppv2[kk2++] = p;
                }
                if (!ppv2.isNull() > 0)
                    pv["paths"] = ppv2;

                v_floors.append(pv);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Block) {
                MapBlock *p = static_cast<MapBlock *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();

                Json::Value ppv;
                auto ps = p->getSpirits();
                int kk = 0;
                for (auto p : ps) {
                    ppv[kk++] = p;
                }
                if (!ppv.isNull())
                    pv["spirits"] = ppv;

                v_blocks.append(pv);
            }
            else if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Group) {
                MapGroup *p = static_cast<MapGroup *>(spirit);
                Json::Value pv;
                pv["id"] = p->getId();
                pv["name"] = p->getName();
                pv["type"] = p->getGroupType();
                Json::Value ppv;
                auto ps = p->getSpirits();
                int kk = 0;
                for (auto p : ps) {
                    ppv[kk++] = p;
                }
                if (!ppv.isNull())
                    pv["spirits"] = ppv;

                v_groups.append(pv);
            }
        }

        if (v_points.size() > 0)
            response["points"] = v_points;
        if (v_paths.size() > 0)
            response["paths"] = v_paths;
        if (v_floors.size() > 0)
            response["floors"] = v_floors;
        if (v_bkgs.size() > 0)
            response["bkgs"] = v_bkgs;
        if (v_blocks.size() > 0)
            response["blocks"] = v_blocks;
        if (v_groups.size() > 0)
            response["groups"] = v_groups;
        response["maxId"] = g_onemap.getMaxId();
    }
    conn->send(response);
}

void MapManager::interTrafficControlStation(SessionPtr conn, const Json::Value &request)
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
        int id = request["id"].asInt();
        MapSpirit *spirit = g_onemap.getSpiritById(id);
        if (spirit == nullptr) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
        }
        else {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point) {
                try {
                    CppSQLite3Buffer bufSQL;
                    MapPoint *station = static_cast<MapPoint *>(spirit);
                    bufSQL.format("update agv_station set locked = 1 where id = %d;", station->getId());
                    g_db.execDML(bufSQL);
                    station->setLocked(true);
                }
                catch (CppSQLite3Exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
                }
                catch (std::exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "info:" << e.what();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0}", e.what());
                }
            }
            else {
                response["result"] = RETURN_MSG_RESULT_FAIL;
                response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
                response["error_info"] = "not path or point";
            }
        }
    }
    conn->send(response);
}

void MapManager::interTrafficReleaseLine(SessionPtr conn, const Json::Value &request)
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
        int id = request["id"].asInt();
        MapSpirit *spirit = g_onemap.getSpiritById(id);
        if (spirit == nullptr) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
        }
        else {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
                try {
                    CppSQLite3Buffer bufSQL;
                    MapPath *path = static_cast<MapPath *>(spirit);
                    bufSQL.format("update agv_line set locked = 0 where id = %d;", path->getId());
                    g_db.execDML(bufSQL);
                    path->setLocked(false);
                }
                catch (CppSQLite3Exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
                }
                catch (std::exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "info:" << e.what();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0}", e.what());
                }
            }
            else {
                response["result"] = RETURN_MSG_RESULT_FAIL;
                response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
                response["error_info"] = "not path or point";
            }
        }
    }
    conn->send(response);
}


void MapManager::interTrafficReleaseStation(SessionPtr conn, const Json::Value &request)
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
        int id = request["id"].asInt();
        MapSpirit *spirit = g_onemap.getSpiritById(id);
        if (spirit == nullptr) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
        }
        else {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Point) {
                try {
                    CppSQLite3Buffer bufSQL;
                    MapPoint *station = static_cast<MapPoint *>(spirit);
                    bufSQL.format("update agv_station set locked = 0 where id = %d;", station->getId());
                    g_db.execDML(bufSQL);
                    station->setLocked(false);
                }
                catch (CppSQLite3Exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
                }
                catch (std::exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "info:" << e.what();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0}", e.what());
                }
            }
            else {
                response["result"] = RETURN_MSG_RESULT_FAIL;
                response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
                response["error_info"] = "not path or point";
            }
        }
    }
    conn->send(response);
}

void MapManager::interTrafficControlLine(SessionPtr conn, const Json::Value &request)
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
        int id = request["id"].asInt();
        MapSpirit *spirit = g_onemap.getSpiritById(id);
        if (spirit == nullptr) {
            response["result"] = RETURN_MSG_RESULT_FAIL;
            response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
        }
        else {
            if (spirit->getSpiritType() == MapSpirit::Map_Sprite_Type_Path) {
                try {
                    CppSQLite3Buffer bufSQL;
                    MapPath *path = static_cast<MapPath *>(spirit);
                    bufSQL.format("update agv_line set locked = 1 where id = %d;", path->getId());
                    g_db.execDML(bufSQL);
                    path->setLocked(true);
                }
                catch (CppSQLite3Exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
                }
                catch (std::exception e) {
                    response["result"] = RETURN_MSG_RESULT_FAIL;
                    response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                    std::stringstream ss;
                    ss << "info:" << e.what();
                    response["error_info"] = ss.str();
                    combined_logger->error("sqlerr code:{0}", e.what());
                }

            }
            else {
                response["result"] = RETURN_MSG_RESULT_FAIL;
                response["error_code"] = RETURN_MSG_ERROR_CODE_UNFINDED;
                response["error_info"] = "not path or point";
            }
        }
    }
    conn->send(response);
}


bool MapManager::addOccuGroup(int groupid, int agvId)
{
    auto group = getGroupById(groupid);
    if(group != nullptr){
        combined_logger->info("group id:{0} change status to lock", groupid);
    }
    else
    {
        combined_logger->info("group id:{0} error", groupid);
    }
    auto sps = group->getSpirits();
    for(auto spirit:sps)
    {
        //TODO 检查当前电梯是否已经被AGV锁定如果已锁定返回错误
        auto line = MapManager::getInstance()->getPathById(spirit);
        if(line != nullptr)
        {
            if(line_occuagvs[line->getId()].size())
            {
                return false;
            }
        }
    }
    for(auto spirit:sps)
    {
        auto line = MapManager::getInstance()->getPathById(spirit);
        if(line != nullptr)
        {
            line_occuagvs[line->getId()].push_back(agvId);
        }
    }
    return true;
}
bool MapManager::freeGroup(int groupid, int agvId)
{
    auto group = getGroupById(groupid);
    if(group != nullptr){
        combined_logger->info("group id:{0} change status to unlock", groupid);
    }
    else
    {
        combined_logger->info("group id:{0} error", groupid);
    }
    auto sps = group->getSpirits();
    for(auto spirit:sps)
    {
        auto line = MapManager::getInstance()->getPathById(spirit);
        if(line != nullptr)
        {
            for (auto itr = line_occuagvs[line->getId()].begin(); itr != line_occuagvs[line->getId()].end(); ) {
                if (*itr == agvId) {
                    itr = line_occuagvs[line->getId()].erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }
    }
    return true;
}
