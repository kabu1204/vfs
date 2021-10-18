//
// Created by yuchengye on 2021/10/18.
//

#ifndef VFS_INODE_H
#include "types.h"
#include <string>
#include <future>
#include "thread"
#define VFS_INODE_H

#pragma pack(1)
struct MetaData
{
    FILE_TYPE fileType;     // 文件类型
    ulong fileSize;         // 文件总字节数
    ulong fileSpace;        // 文件所占空间
    char fileName[128];     // 文件名
    ushort owner;           // 文件Owner ID
    ushort mode;            // 文件读写可执行权限
    ushort group;           // 文件Group ID
    ulong ctime;            // i节点上一次变动的时间
    ulong mtime;            // 文件内容上一次变动的时间
    ulong atime;            // 文件上一次打开的时间
    ulong n_link;           // 文件链接数
};
#pragma pack()


class inode {
public:
    ulong pointers[MAX_BLOCKS_PER_NODE];    // 存储i节点指向的磁盘块号
    ulong allocated_block_n;
    MetaData metaData;
    inode(char _filename[128], ushort _owner, ushort _group);
};


#endif //VFS_INODE_H
