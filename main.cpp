#include "main.h"
#include "types.h"
#include "superblock.h"
#include "inode.h"
#include <iostream>
#include <cstdio>

std::vector<inode> in_mem_inodes;

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

int init_Superblock(std::ofstream* disk, superblock_c* spb) {
    /*
     * 功能：初始化超级块
     */
    char fill_char = '\0';  // 空白填充字符
    spb = new superblock_c(0, MAX_SPACE, BLOCK_SIZE, MAX_INODE_NUM);  // 建立新的superblock

    ulong superblock_size_n = sizeof(spb->s_block);  // superblock实际所占字节数
    ulong superblock_block_n;    // superblock所占block数量
    superblock_block_n = (superblock_size_n % BLOCK_SIZE == 0) ? superblock_size_n/BLOCK_SIZE : superblock_size_n/BLOCK_SIZE + 1;

    // 以下是手动为superblock分配空间
    spb->s_block.free_block_num -= superblock_block_n;
    spb->s_block.free_space -= superblock_block_n * BLOCK_SIZE;    // 剩余空间需要减去superblock所占空间。
    spb->s_block.inode_start_addr = spb->s_block.superblock_start_addr + superblock_block_n * spb->s_block.block_size;   // i节点起始地址是superblock结束地址+1
    spb->s_block.superblock_size = superblock_size_n;    // superblock占用字节数
    spb->s_block.last_p = superblock_block_n;
    // superblock占用磁盘块置为已分配状态
    for(int i=0; i<superblock_block_n; ++i)
        spb->s_block.bitmap.set(i);


    disk -> write((char *)&(spb->s_block), superblock_size_n);   // 写入superblock
    for(int i=0;i<(superblock_block_n*BLOCK_SIZE - superblock_size_n);++i)
        disk -> write(&fill_char, sizeof(fill_char));   //剩余空间填入空白字符

    return 1;
}

int init_inodeSpace(std::ofstream* disk, superblock_c* spb) {
    /*
     * 初始化inode区
     */
    MetaData t;
    ulong inodes_size_n = MAX_INODE_NUM * sizeof(t);
    ulong inodes_block_n = (inodes_size_n % BLOCK_SIZE == 0) ? inodes_size_n/BLOCK_SIZE : inodes_size_n/BLOCK_SIZE + 1;


    // 以下是手动为inodes区分配空间
    spb->s_block.free_block_num -= inodes_block_n;
    spb->s_block.free_space -= inodes_block_n * BLOCK_SIZE;    // 剩余空间需要减去inodes所占空间。
    spb->s_block.root_dir_start_addr = spb->s_block.inode_start_addr + inodes_block_n * BLOCK_SIZE;   // 根目录区起始地址是inodes结束地址+1
    // inodes占用磁盘块置为已分配状态
    for(ulong i=spb->s_block.last_p; i<inodes_block_n; ++i)
        spb->s_block.bitmap.set(i);
    spb->s_block.last_p += inodes_block_n;

    disk->seekp(spb->s_block.inode_start_addr);
    for(int i=0;i<MAX_INODE_NUM;++i){
        disk->write((char *)&t, sizeof(t));
    }

    return 1;
}