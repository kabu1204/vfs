#ifndef VFS_TYPES_H
#define VFS_TYPES_H
#include <cstdint>

#define MAX_SPACE 104857600          // 磁盘空间 100MB
#define BLOCK_SIZE 1024              // 磁盘块大小 1KB
#define MAX_INODE_NUM 2048           // 最多i节点数
#define MAX_BLOCKS_PER_NODE 256      // 每个i节点指向的最多磁盘块数
#define MAX_CHILD_PER_DIR 128        // 每个目录下最多子文件/目录数
#define MAX_FILENAME_LEN 128         // 文件名最大长度
#define MAX_USER_NAME_LEN 64         // 用户名最大长度
#define MAX_USER_NUM 64              // 最大用户数
#define IPC_BUFFER_SIZE 1024         // IPC的缓冲区大小
#define uint32 uint32_t              // 主要用于地址、和磁盘块号。因为磁盘空间是100MB，因此定为uint32
#define ushort unsigned short        // 主要用在i节点号上

#define IPC_LOGIN_REQ 0
#define IPC_CLI2SERVER 1
#define IPC_SERVER2CLI 2
#define IPC_CMD 3
#define IPC_OK 4
#define IPC_END 5

struct msg_proto{
    long mtype;
    char data[IPC_BUFFER_SIZE];
};

enum FILE_TYPE{
    NORMAL,
    DIR,
    LINK
};

#endif //VFS_TYPES_H