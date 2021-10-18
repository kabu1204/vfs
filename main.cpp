#include "main.h"
#include "types.h"
#include "superblock.h"
#include <iostream>
#include <cstdio>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}

int Init() {
    /*
     * 功能：初始化磁盘
     */
    char fill_char = '\0';  // 空白填充字符

    // 打印提示信息
    std::cout << "Creating file system and formatting the disk space\n";
    std::printf("Total disk space: %.2fMB", double(MAX_SPACE)/1024/1024);

    // 建立虚拟磁盘
    std::ofstream disk("vfs.disk", std::ios::out | std::ios::binary);
    for(unsigned long i=0; i<MAX_SPACE; ++i)
        disk.write(&fill_char, sizeof(fill_char));  // 写入填充字符

    std::cout << "formatting finished\n";

    /*
     * TODO: superblock and init root dir
     */
    return 1;
}

int Format(std::ofstream* disk) {
    /*
     * 功能：格式化磁盘
     */
    char fill_char = '\0';  // 空白填充字符
    superblock_c s_block(0, MAX_SPACE, BLOCK_SIZE, MAX_INODE_NODE);  // 建立新的superblock

    ulong superblock_size_n = sizeof(s_block.s_block);  // superblock实际所占字节数
    ulong superblock_block_n;    // superblock所占block数量
    superblock_block_n = (superblock_block_size_n % BLOCK_SIZE == 0) ? superblock_size_n/BLOCK_SIZE : superblock_size_n/BLOCK_SIZE + 1;
    s_block.s_block.free_block_num -= superblock_block_n;
    s_block.s_block.free_space -= superblock_block_n*s_block.s_block.block_size;    // 剩余空间需要减去superblock所占空间。
    s_block.s_block.inode_start_addr = s_block.s_block.superblock_start_addr + superblock_block_n*s_block.s_block.block_size;
    s_block.s_block.superblock_size = superblock_size_n;


    disk -> write((char *)&(s_block.s_block), superblock_size_n);   // 写入superblock
    for(int i=0;i<(superblock_block_n*BLOCK_SIZE - superblock_size_n);++i)
        disk -> write(&fill_char, sizeof(fill_char));   //剩余空间填入空白字符

    return 1;
    
}