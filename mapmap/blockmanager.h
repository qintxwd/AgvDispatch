#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H
#include "../common.h"
#include "../utils/noncopyable.h"

class BlockManager;
using BlockManagerPtr = std::shared_ptr<BlockManager>;

class AgvOccuSpirits{
public:
    AgvOccuSpirits(int _agvid);
    AgvOccuSpirits(const AgvOccuSpirits & b);

    AgvOccuSpirits &operator =(const AgvOccuSpirits &b);

    void addSpirit(int spiritId);
    void removeSpirit(int spiritId);

    bool empty();
    int getAgvid();
private:
    int agvid;
    std::vector<int> spirits;
};

class BBlock{
public:
    BBlock(int bid);
    BBlock(const BBlock & b);

    BBlock &operator =(const BBlock &b);

    void addOccu(int agvid,int spiritid);
    void removeOccu(int agvid,int spiritid);

    bool passable(int agvid);

    int getBlockId();

    void print();
private:
    int block_id;
    std::vector<AgvOccuSpirits> agv_spirits;
};

class BlockManager: public noncopyable, public std::enable_shared_from_this<BlockManager>
{
public:
    static BlockManagerPtr getInstance() {
        static BlockManagerPtr m_inst = BlockManagerPtr(new BlockManager());
        return m_inst;
    }

    bool tryAddBlockOccu(std::vector<int> blocks,int agvId,int spiritId);
    void freeBlockOccu(std::vector<int> blocks,int agvId, int spiritId);

    bool blockPassable(std::vector<int> blocks, int agvId);
    void clear();
    void printBlock();
    void test();
private:
    BlockManager();

    std::mutex blockMtx;
    std::vector<BBlock> bblocks;
};

#endif // BLOCKMANAGER_H
