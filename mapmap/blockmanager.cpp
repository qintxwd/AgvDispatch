#include "blockmanager.h"

AgvOccuSpirits::AgvOccuSpirits(int _agvid):
    agvid(_agvid)
{

}

AgvOccuSpirits::AgvOccuSpirits(const AgvOccuSpirits & b):
    agvid(b.agvid)
{
    spirits = b.spirits;
}

AgvOccuSpirits &AgvOccuSpirits::operator =(const AgvOccuSpirits &b)
{
    AgvOccuSpirits c(b.agvid);
    c.spirits = b.spirits;
    return c;
}

void AgvOccuSpirits::addSpirit(int spiritId)
{
    if(std::find( spirits.begin(),spirits.end(),spiritId) == spirits.end())
        spirits.push_back(spiritId);
}

void AgvOccuSpirits::removeSpirit(int spiritId)
{
    for(auto itr = spirits.begin();itr!=spirits.end();){
        if(*itr == spiritId){
            itr = spirits.erase(itr);
        }else
            ++itr;
    }
}

bool AgvOccuSpirits::empty()
{
    return spirits.empty();
}

int AgvOccuSpirits::getAgvid()
{
    return agvid;
}

BBlock::BBlock(int bid):
    block_id(bid)
{
}

BBlock::BBlock(const BBlock & b):
    block_id(b.block_id)
{
    block_id = b.block_id;
    agv_spirits = b.agv_spirits;
}

BBlock &BBlock::operator =(const BBlock &b)
{
    BBlock c(b.block_id);
    c.agv_spirits = b.agv_spirits;
    return c;
}

void BBlock::addOccu(int agvid,int spiritid)
{
    bool alreadAdd = false;
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->addSpirit(spiritid);
            alreadAdd = true;
            break;
        }
    }
    if(!alreadAdd){
        AgvOccuSpirits a(agvid);
        a.addSpirit(spiritid);
        agv_spirits.push_back(a);
    }
}

void BBlock::removeOccu(int agvid,int spiritid)
{
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->removeSpirit(spiritid);
            break;
        }
    }
}

bool BBlock::passable(int agvid)
{
    for(auto aos:agv_spirits){
        if(aos.getAgvid() == agvid)continue;
        if(!aos.empty())return false;
    }
    return true;
}

int BBlock::getBlockId()
{
    return block_id;
}

void BBlock::print()
{
    for(auto aos:agv_spirits){
        if(!aos.empty())
        {
            combined_logger->debug("blockid:{0} ",getBlockId());
            combined_logger->debug("agv:{0} ",aos.getAgvid());
        }
    }
}

BlockManager::BlockManager()
{

}

bool BlockManager::tryAddBlockOccu(std::vector<int> blocks,int agvId,int spiritId)
{
    if(spiritId>2000){
        combined_logger->debug("spiritId={0}",spiritId);
    }
    if(!blockPassable(blocks,agvId))return false;


    UNIQUE_LCK(blockMtx);
    for(auto block:blocks)
    {
        bool alreadAdd = false;
        for(auto itr=bblocks.begin();itr!=bblocks.end();++itr)
        {
            if(itr->getBlockId() == block){
                itr->addOccu(agvId,spiritId);
                alreadAdd = true;
                break;
            }
        }

        if(!alreadAdd){
            BBlock bb(block);
            bb.addOccu(agvId,spiritId);
            bblocks.push_back(bb);
        }
    }

    return true;
}
void BlockManager::freeBlockOccu(std::vector<int> blocks,int agvId, int spiritId)
{
    UNIQUE_LCK(blockMtx);
    for(auto block:blocks)
    {
        for(auto itr=bblocks.begin();itr!=bblocks.end();++itr)
        {
            if(itr->getBlockId() == block){
                itr->removeOccu(agvId,spiritId);
            }
        }
    }
}

bool BlockManager::blockPassable(std::vector<int> blocks, int agvId)
{
    UNIQUE_LCK(blockMtx);
    for(auto block:blocks)
    {
        for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
            if(itr->getBlockId() == block){
                if(!itr->passable(agvId))return false;
            }
        }
    }
    return true;
}

void BlockManager::clear()
{
    UNIQUE_LCK(blockMtx);
    bblocks.clear();
}

void BlockManager::printBlock()
{
    //TODO
    UNIQUE_LCK(blockMtx);
    combined_logger->debug("===============================================");
    for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
        itr->print();
    }
    combined_logger->debug("===============================================");
}

void BlockManager::test()
{
    std::vector<int> bs;
    bs.push_back(101);
    bs.push_back(102);
    bs.push_back(103);

    std::vector<int> bss;
    bss.push_back(101);
    bss.push_back(103);

    tryAddBlockOccu(bs,1,1001);
    printBlock();
    tryAddBlockOccu(bss,1,1002);
    printBlock();
    freeBlockOccu(bs,1,1002);
    printBlock();
    freeBlockOccu(bss,1,1001);
    printBlock();
}

