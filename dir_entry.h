#ifndef VFS_DIR_ENTRY_H
#define VFS_DIR_ENTRY_H
#include "types.h"

struct dir_entry
{
    ushort fileNum = 0;         // 当前目录下文件（包括目录，不包含.和..两个特殊目录）的数量
    ushort inode_arr[MAX_CHILD_PER_DIR+2] = {0};    // 当前目录下文件对应的i节点号，第0位是当前目录对应的i节点号，第1位是上级目录对应的i节点号
    ushort totalSize = 0;       // 当前目录所占总空间，包括子文件
    dir_entry(ushort curr_inode, ushort prev_inode);
};



#endif //VFS_DIR_ENTRY_H
