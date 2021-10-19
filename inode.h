#ifndef VFS_INODE_H
#define VFS_INODE_H
#include "types.h"
#include <string>
#include <future>
#include <vector>
#include <thread>
#include <map>
#include "superblock.h"

#pragma pack(1)
struct inode
{
    bool allocated = false;         // 标识该i节点是否已被分配
    ushort id = 0;            // i节点号
    FILE_TYPE fileType = NORMAL;     // 文件类型
    ulong fileSize = 0;         // 文件总字节数
    ulong fileSpace = 0;        // 文件所占空间
    char fileName[MAX_FILENAME_LEN] = "\0";     // 文件名
    ushort owner = 0;           // 文件Owner ID
    ushort mode = 755;            // 文件读写可执行权限
    ushort group = 0;           // 文件Group ID
    ulong ctime = 0;            // i节点上一次变动的时间
    ulong mtime = 0;            // 文件内容上一次变动的时间
    ulong atime = 0;            // 文件上一次打开的时间
    ushort n_link = 0;           // 文件链接数
    ulong blocks[MAX_BLOCKS_PER_NODE]={0};    // 存储i节点指向的磁盘块号
    ushort allocated_block_n = 0;             // 该i节点指向的磁盘块总数目
};
#pragma pack()


class inode_ {
public:
    inode(char _filename[MAX_FILENAME_LEN], ushort _owner, ushort _group, FILE_TYPE _filetype, superblock_c* spb);
    ~inode();
private:
    bool _del(superblock_c* spb);
};

class inode_mgmt{
public:
    ushort free_inode_num;                  // 可分配的i节点数
    std::map<ushort, inode> in_mem_inodes;  // 已分配的inode，调入内存中
    std::vector<ushort> reclaimed_inodes;   // 暂存已释放的inodes的节点号，以便快速分配
    std::bitset<MAX_INODE_NUM> bitmap;      // i节点的位图
    ushort last_p;                          // 记录上次分配的i节点号
    std::pair<ushort, bool> alloc_inode();

};


#endif //VFS_INODE_H
