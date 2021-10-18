#include "superblock.h"
#include "types.h"
#include <cstdio>


superblock_c::superblock_c(ulong _superblock_start_addr, ulong _max_space, ulong _block_size, ulong _inode_num) {
    s_block.max_space = _max_space;
    s_block.block_size = _block_size;
    s_block.max_block_num = _max_space/_block_size;
    s_block.max_inode_num = _inode_num;
    s_block.available_space = s_block.max_block_num * _block_size;

    s_block.free_space = s_block.available_space;
    s_block.free_block_num = s_block.max_block_num;
    s_block.free_inode_num = s_block.max_inode_num;

    s_block.superblock_start_addr = _superblock_start_addr;
}

void superblock_c::print() {
    /*
     * 打印超级块信息
     */
    std::printf("-------Superblock Info-------");
    std::printf("Max space:\t%luBytes\n", s_block.max_space);
    std::printf("Available space:\t%luBytes\n", s_block.available_space);
    std::printf("Free space:\t%luBytes(%.2f%%)\n", s_block.free_space, double(s_block.free_space)/s_block.available_space);
    std::printf("Block size:\t%luBytes\n", s_block.block_size);
    std::printf("Max block num:\t%lu\n", s_block.max_block_num);
    std::printf("Free block num:\t%lu(%.2f%%)\n", s_block.free_block_num, double(s_block.free_block_num)/s_block.max_block_num);

    std::printf("Max inode num:\t%lu\n", s_block.max_inode_num);
    std::printf("Free inode num:\t%lu\n", s_block.free_inode_num);
    std::printf("-------Superblock Info-------");
}

ulong superblock_c::alloc_n_blocks(ulong n) {
    /*
     * 接口：
     *   安全地申请n个磁盘块，可供外部调用。
     */
    return 0;
}

void superblock_c::_alloc_blocks_addr(ulong n, ulong start_addr) {
    /*
     * 私有方法：
     *   从start_addr开始分配n个磁盘块，非安全，私有成员函数，仅限内部调用。
     */
    s_block.free_block_num -= n;
    s_block.free_space -= n*s_block.block_size;
}

void superblock_c::_write_bin(const char *s, ulong n, ulong start_addr) {
    /*
     * 内部方法：
     *   从start_addr处开始将n字节的s写入磁盘
     */

}
