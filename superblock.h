//
// Created by yuchengye on 2021/10/18.
//

#ifndef VFS_SUPERBLOCK_H
#include "types.h"
#include <bitset>
#include <vector>
#define VFS_SUPERBLOCK_H

#pragma pack(1)
struct superblock{
    ulong max_space;                // 磁盘空间总字节数
    ulong block_size;               // 块大小，单位为Byte
    ulong max_block_num;            // 最多块数量
    ulong max_inode_num;            // 最多i节点数量
    ulong available_space;          // 最大可用字节数 = 最大块数量 * 块大小，包含已用的和剩余的

    ulong free_space;               // 剩余可用空间 = 最大可用字节数-已用字节数 = 可用块数量*块大小。单位：Byte
    ulong free_inode_num;           // 剩余可分配i节点数
    ulong free_block_num;           // 剩余可分配的block数量

    ulong superblock_start_addr;    // 超级块起始地址，对齐方式：Byte
    ulong superblock_size;          // 超级块实际所占字节数
    ulong inode_start_addr;         // i节点起始地址，对齐方式：Byte
    ulong root_dir_start_addr;      // 根目录区起始地址
    std::bitset<MAX_SPACE/BLOCK_SIZE> bitmap;    // bitset，每一位对应一个block，0表示未分配，1表示已分配
    ulong last_p;                   // 最近一次被分配的磁盘块号
};
#pragma pack()

class superblock_c {
    /*
     * 包含了超级块的基本信息以及接口函数
     */
public:
    superblock s_block;

    std::vector<ulong> reclaimed_blocks;    // 回收的磁盘块暂存在这里，以便快速分配。

    superblock_c(ulong _superblock_start_addr, ulong _max_space, ulong _block_size = BLOCK_SIZE, ulong _inode_num = MAX_INODE_NUM);

    void load_super_block();

    bool reclaim_block(ulong block_id);

    void print();

    std::pair<std::vector<ulong>, bool > alloc_n_blocks(unsigned short n);

    std::pair<ulong, bool > alloc_block();

private:
    void _alloc_blocks_addr(ulong n, ulong start_addr);
    void _write_bin(const char* s, ulong n, ulong start_addr);
};


#endif //VFS_SUPERBLOCK_H