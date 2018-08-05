#include "taskmaker.h"
#include "qunchuang/qunchuangtaskmaker.h"
#include "qingdao/qingdaotaskmaker.h"
#include "Dongyao/dytaskmaker.h"
#include "Anting/attaskmaker.h"
#include "common.h"

TaskMaker::TaskMaker()
{

}

TaskMaker::~TaskMaker()
{

}

TaskMaker* TaskMaker::getInstance()
{
    if(GLOBAL_AGV_PROJECT == AGV_PROJECT_QINGDAO){
		//不同的AGV，调用不同的子类，即可
		static QingdaoTaskMaker *m_ins = nullptr;
		if (m_ins == nullptr) {
			m_ins = new QingdaoTaskMaker();
		}
		return m_ins;
    }else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_QUNCHUANG){
        //不同的AGV，调用不同的子类，即可
        static QunChuangTaskMaker *m_ins = nullptr;
        if(m_ins == nullptr){
            m_ins = new QunChuangTaskMaker();
        }
        return m_ins;
    }
    else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO){
        static DyTaskMaker *m_ins = nullptr;
        if(m_ins == nullptr){
            m_ins = new DyTaskMaker("127.0.0.1", 12345);
        }
        return m_ins;
    }
    else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING){
        static AtTaskMaker *m_ins = nullptr;
        if(m_ins == nullptr){
            m_ins = new AtTaskMaker("127.0.0.1", 12345);
        }
        return m_ins;
    }
    return nullptr;
}

//群创接口//非群创不需要重写
void TaskMaker::makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info)
{

}
