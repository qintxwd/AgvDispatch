#ifndef QUNCHUANGTASKMAKER_H
#define QUNCHUANGTASKMAKER_H

#include "../taskmaker.h"
#include "qunchuangtcsconnection.h"

class QunChuangTaskMaker : public TaskMaker
{
public:
    QunChuangTaskMaker();
    ~QunChuangTaskMaker();
    virtual void init();
    virtual void makeTask(SessionPtr conn, const Json::Value &request);

    //all_floor_info: 为3层升降货架是否有料信息
    /* info 为3层升降货架每一层是否有料信息
     * 第3层 :             0        0        0        0        1        1        1        1
     * 第2层 :             0        0        1        1        0        0        1        1
     * 第1层 :             0        1        0        1        0        1        0        1
     * all_floor_info : 0x0000   0x0001   0x0002   0x0003   0x0004   0x0005   0x0006   0x0007
     */
    virtual void makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info);

private:
    QunChuangTcsConnection *tcsConnection;
};

#endif // QUNCHUANGTASKMAKER_H
