#ifndef VFS_SUPERBLOCK_H
#define VFS_SUPERBLOCK_H
#include "types.h"
#include "ioservice.h"
#include <bitset>
#include <vector>

#pragma pack(1)
struct superblock{
    uint32 max_space;                // 磁盘空间总字节数
    uint32 block_size;               // 块大小，单位为Byte
    uint32 max_block_num;            // 最多块数量
    uint32 max_inode_num;            // 最多i节点数量
    uint32 available_space;          // 最大可用字节数 = 最大块数量 * 块大小，包含已用的和剩余的

    uint32 free_space;               // 剩余可用空间 = 最大可用字节数-已用字节数 = 可用块数量*块大小。单位：Byte
    uint32 free_inode_num;           // 剩余可分配i节点数
    uint32 free_block_num;           // 剩余可分配的block数量

    uint32 superblock_start_addr;    // 超级块起始地址，默认为0，对齐方式：Byte
    uint32 superblock_size;          // 超级块实际所占字节数
    uint32 inode_start_addr;         // i节点起始地址，对齐方式：Byte
    uint32 root_dir_start_addr;      // 根目录区起始地址
    std::bitset<MAX_SPACE/BLOCK_SIZE> bitmap;    // bitset，每一位对应一个block，0表示未分配，1表示已分配
    uint32 last_p;                   // 最近一次被分配的磁盘块号
};
#pragma pack()

class superblock_c {
    /*
     * 包含了超级块的基本信息以及接口函数
     */
public:
    superblock s_block;

    superblock_c(uint32 _superblock_start_addr, uint32 _max_space, uint32 _block_size = BLOCK_SIZE, uint32 _inode_num = MAX_INODE_NUM);

    ~superblock_c();

    void set_io_context(ioservice *_io_context);

    void load_super_block(std::ifstream *disk);

    bool reclaim_block(uint32 block_id);

    void print();

    std::pair<std::vector<uint32>, bool > alloc_n_blocks(ushort n);

    std::pair<uint32, bool > alloc_block();

private:
    std::vector<uint32> reclaimed_blocks;    // 回收的磁盘块暂存在这里，以便快速分配。
    ioservice *io_context;      // ioservice

    void store_superblock();
    void _alloc_blocks_addr(uint32 n, uint32 start_addr);
    void _write_bin(const char* s, uint32 n, uint32 start_addr);
};


#endif //VFS_SUPERBLOCK_H
