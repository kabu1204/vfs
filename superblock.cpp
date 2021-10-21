#include "superblock.h"
#include "types.h"
#include <utility>
#include <cstdio>


superblock_c::superblock_c(uint32 _superblock_start_addr, uint32 _max_space, uint32 _block_size, uint32 _inode_num) {
    s_block.max_space = _max_space;
    s_block.block_size = _block_size;
    s_block.max_block_num = _max_space/_block_size;
    s_block.max_inode_num = _inode_num;
    s_block.available_space = s_block.max_block_num * _block_size;

    s_block.free_space = s_block.available_space;
    s_block.free_block_num = s_block.max_block_num;
    s_block.free_inode_num = s_block.max_inode_num;

    s_block.superblock_start_addr = _superblock_start_addr;
    s_block.superblock_size = sizeof(s_block);
    s_block.last_p = 0;
}

void superblock_c::print() {
    /*
     * 打印超级块信息
     */
    std::printf("-------Superblock Info-------\n");
    std::printf("Max space:\t\t\t%u Bytes\n", s_block.max_space);
    std::printf("Available space:\t%u Bytes\n", s_block.available_space);
    std::printf("Free space:\t\t\t%u Bytes(%.2f%%)\n", s_block.free_space, 100*double(s_block.free_space)/s_block.available_space);
    std::printf("Block size:\t\t\t%u Bytes\n", s_block.block_size);
    std::printf("Max block num:\t\t%u\n", s_block.max_block_num);
    std::printf("Free block num:\t\t%u(%.2f%%)\n", s_block.free_block_num, 100*double(s_block.free_block_num)/s_block.max_block_num);

    std::printf("Max inode num:\t\t%u\n", s_block.max_inode_num);
    std::printf("Free inode num:\t\t%u(%.2f%%)\n", s_block.free_inode_num, 100*double(s_block.free_block_num)/s_block.max_block_num);

    std::printf("Superblock start address:\t%u\n", s_block.superblock_start_addr);
    std::printf("inodes start address:\t%u\n", s_block.inode_start_addr);
    std::printf("Root dir_entry start address:\t%u\n", s_block.root_dir_start_addr);
//    std::printf("-------Superblock Info-------\n");
}

std::pair<std::vector<uint32>, bool> superblock_c::alloc_n_blocks(ushort n) {
    /*
     * 接口：
     *   安全地申请n个磁盘块，可供外部调用。
     *   ret.second为false时，申请失败。
     */
    std::pair<std::vector<uint32>, bool> ret(std::vector<uint32>(), false);

    if(n>MAX_BLOCKS_PER_NODE){  // n不能超过每个i节点能索引的最大磁盘块数量
        std::cerr<<"Cannot allocate "<<n<<" blocks at once! (Limits: "<<MAX_BLOCKS_PER_NODE<<")\n";
        return ret;
    }
    if(n>s_block.free_block_num){   // n不能大于剩余block数量
        std::cerr<<"Not enough blocks to allocate! ("<<s_block.free_block_num<<" left)\n";
        return ret;
    }

    std::pair<uint32, bool> res;
    for(ushort i=0; i<n; ++i)
    {
        res = alloc_block();
        if(res.second){
            ret.first.push_back(res.first);
        }
        else{
            // 分配失败，开始回滚
            for(uint32 & block_id : ret.first) {
                assert(reclaim_block(block_id));
            }
            return ret;
        }
    }
    ret.second = true;
    return ret;
}

void superblock_c::_alloc_blocks_addr(uint32 n, uint32 start_addr) {
    /*
     * 私有方法：
     *   从start_addr开始分配n个磁盘块，非安全，私有成员函数，仅限内部调用。
     */
    s_block.free_block_num -= n;
    s_block.free_space -= n*s_block.block_size;
}

void superblock_c::_write_bin(const char *s, uint32 n, uint32 start_addr) {
    /*
     * 内部方法：
     *   从start_addr处开始将n字节的s写入磁盘
     */

}

std::pair<uint32, bool> superblock_c::alloc_block() {
    /*
     * 接口：
     *   安全地选择并返回一个未分配的磁盘块号
     *   首先检查是否有回收的暂存磁盘块号可以分配，若没有才检索bitmap来获取可用块号
     *   返回值为(0, false)时表示分配失败
     */
    std::pair<uint32, bool> ret(0, false);
    if(s_block.free_block_num==0) return ret;

    if(!reclaimed_blocks.empty())
    {
        // 若reclaimed_blocks非空，则有暂存的磁盘块可以立马分配，无需再查bitmap
        uint32 block_id = reclaimed_blocks.back();
        reclaimed_blocks.pop_back();
        s_block.bitmap.set(block_id);   // bitmap置1，代表已分配
        s_block.last_p = block_id;
        s_block.free_block_num -= 1;
        ret.first = block_id;
        ret.second = true;
        return ret;
    }

    for(uint32 i=s_block.last_p; i < s_block.max_block_num; ++i) {
        // 先从last_p处向后找
        if (!s_block.bitmap.test(i)) {
            s_block.bitmap.set(i);
            s_block.last_p = i;
            s_block.free_block_num -= 1;
            ret.first = i;
            ret.second = true;
            return ret;
        }
    }

    // 若last_p之后没有空闲块号，则向前找
    for(uint32 i= s_block.last_p - 1; i >= 0; --i)
    {
        if (!s_block.bitmap.test(i)) {
            s_block.bitmap.set(i);
            s_block.last_p = i;
            s_block.free_block_num -= 1;
            ret.first = i;
            ret.second = true;
            return ret;
        }
    }

    return ret;
}

bool superblock_c::reclaim_block(uint32 block_id) {
    /*
     * 接口：
     *   回收block_id对应的磁盘块
     *   TODO: 是否需要用'\0'覆写block区域
     */
    if(block_id>=s_block.max_block_num || block_id<0) return false;   // 非法的block_id
    if(!s_block.bitmap.test(block_id))  return false;   // 要回收的磁盘块是未分配状态；


    reclaimed_blocks.push_back(block_id);               // 暂存被回收的磁盘块号，以便下次快速分配
    s_block.bitmap.reset(block_id);
    s_block.free_block_num += 1;
    s_block.free_space += s_block.block_size;


    return true;
}

void superblock_c::load_super_block(std::ifstream *disk) {
    /*
     * 接口：
     *   读取已保存在磁盘上的superblock
     */
    disk->read((char *)&s_block, s_block.superblock_size);
}

superblock_c::~superblock_c() {
    /*
     * 析构函数：
     *    释放前将超级块信息写回磁盘
     */
    store_superblock();
}

void superblock_c::store_superblock() {
    /*
     * 内部方法：
     *    将superblock写回到磁盘中
     */
    io_context->write((char *)&s_block, s_block.superblock_start_addr, s_block.superblock_size);
    std::cout<<"Successfully wrote superblock back to disk!\n";
}

void superblock_c::set_io_context(ioservice *_io_context) {
    /*
     * 接口：
     *   设置io服务，用于读写磁盘，主要为了将superblock写回磁盘
     */
    io_context = _io_context;
}
