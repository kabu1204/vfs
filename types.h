//
// Created by yuchengye on 2021/10/18.
//

#ifndef VFS_TYPES_H
#define VFS_TYPES_H
#include <cstdint>

#define MAX_SPACE 104857600         // 磁盘空间 100MB
#define BLOCK_SIZE 1024             // 磁盘块大小 1KB
#define MAX_INODE_NUM 2048          // 最多i节点数
#define MAX_BLOCKS_PER_NODE 256     // 每个i节点指向的最多磁盘块数
#define MAX_CHILD_PER_DIR 128       // 每个目录下最多子文件/目录数
#define MAX_FILENAME_LEN 128        // 文件名最大长度
#define MAX_USER_NAME 64            // 用户名最大长度
#define uint32 uint32_t              // 主要用于地址、和磁盘块号。因为磁盘空间是100MB，因此定为uint32
#define ushort unsigned short       // 主要用在i节点号上

enum FILE_TYPE{
    NORMAL,
    DIR
};

#endif //VFS_TYPES_H