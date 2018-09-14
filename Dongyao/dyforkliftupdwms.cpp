#include "dyforkliftupdwms.h"
#include <cassert>
#include "../common.h"
#include "dyforklift.h"
#include "dytaskmaker.h"

DyForkliftUpdWMS::DyForkliftUpdWMS(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params),
    m_store_no(""),
    m_storage_no(""),
    m_key_part_no(""),
    m_type(0)
{
    if(params.size()>=4){
        m_store_no = params[0];
        m_storage_no = params[1];
        m_type = stringToInt(params[2]);
        m_key_part_no = params[3];
    }
}

void DyForkliftUpdWMS::beforeDoing(AgvPtr agv)
{
    //DONOTHING
}

void DyForkliftUpdWMS::doing(AgvPtr agv)
{
    if(agv->type() != DyForklift::Type){
        //other agv,can not excute dy forklift charge
        bresult = true;
        return ;
    }

    //DyForkliftPtr jap = std::static_pointer_cast<DyForklift>(agv);
    combined_logger->info("dothings-updatewms={0}_{1}:{2}", m_store_no, m_storage_no, m_type);
    DyTaskMaker* dytaskmaker =(DyTaskMaker*)(TaskMaker::getInstance());
    dytaskmaker->finishTask(m_store_no, m_storage_no, m_type, m_key_part_no, agv->getId());
    bresult = true;
}


void DyForkliftUpdWMS::afterDoing(AgvPtr agv)
{
    //DONOTHING
    combined_logger->info("dothings-updatewms done");
}

bool DyForkliftUpdWMS::result(AgvPtr agv)
{
    return bresult;
}
