//
// Created by yuchengye on 2021/10/18.
//

#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#define MAX_SPACE 104857600
#define BLOCK_SIZE 1024
#define MAX_INODE_NUM 4096
#define MAX_BLOCKS_PER_NODE 384
#define MAX_CHILD_PER_DIR 128
#define MAX_FILENAME_LEN 128
#define ulong unsigned long
#define ushort unsigned short

enum FILE_TYPE{
    NORMAL,
    DIR
};

#endif //VFS_TYPES_H