#ifndef VFS_INODE_H
#define VFS_INODE_H
#include "types.h"
#include <string>
#include <future>
#include <vector>
#include <thread>
#include <map>
#include "superblock.h"
#include "ioservice.h"
#include "users.h"
#include "dir_entry.h"

class vfstream;     // 因为需要声明vfstream是inode_mgmt的友元类，因此在这声明类vfstream，定义在vfstream.h中

#pragma pack(1)
struct inode
{
    bool allocated = false;         // 标识该i节点是否已被分配
    ushort id = 0;            // i节点号
    FILE_TYPE fileType = NORMAL;     // 文件类型
    uint32 fileSize = 0;         // 文件总字节数
    uint32 fileSpace = 0;        // 文件所占空间
    char fileName[MAX_FILENAME_LEN] = "\0";     // 文件名
    ushort owner = 0;           // 文件Owner ID
    ushort group = 0;           // 文件所属Group ID
    ushort mode = 755;            // 文件读写可执行权限
    uint32 ctime = 0;            // i节点上一次变动的时间
    uint32 mtime = 0;            // 文件内容上一次变动的时间
    uint32 atime = 0;            // 文件上一次打开的时间
    ushort n_link = 0;           // 文件链接数
    uint32 blocks[MAX_BLOCKS_PER_NODE]={0};    // 存储i节点指向的磁盘块号
    ushort allocated_block_n = 0;             // 该i节点指向的磁盘块总数目
};
#pragma pack()


class inode_mgmt{
public:
    uint32 inode_start_addr;                 // inode区起始地址
    uint32 inode_size;                       // 每个inode的实际大小，单位Byte
    std::map<ushort, inode> in_mem_inodes;  // 已分配的inode，调入内存中
    std::vector<ushort> reclaimed_inodes;   // 暂存已释放的inodes的节点号，以便快速分配
    std::bitset<MAX_INODE_NUM> bitmap;      // i节点的位图
    ushort last_p;                          // 记录上次分配的i节点号
    superblock_c* spb;                      // 超级块
    ioservice* io_context;                  // 读写服务

    friend class vfstream;
    friend std::pair<ushort, bool> path2inode_id(const std::string& path, inode_mgmt* inode_manager, ushort _uid, ushort _gid);

    explicit inode_mgmt(superblock_c *_spb, ioservice *_io_context);
    ~inode_mgmt();
    bool load_inode(ushort inode_id);
    bool store_inode(ushort inode_id);
    std::pair<ushort, bool> mkdir(std::string _filepath, unsigned short _owner, unsigned short _group);
    std::pair<ushort, bool> mkfile(std::string _filepath, unsigned short _owner, unsigned short _group);
    size_t getsizeof(std::string _filepath, ushort _uid, ushort _gid);
    std::pair<std::string, bool> get_protection_code(unsigned short inode_id);

private:
    bool reclaim_inode(ushort inode_id);
    std::pair<ushort, bool> alloc_inode();  // 分配一个i节点
    bool write_data(ushort inode_id, char *data, uint32 n);
    std::pair<std::string, bool> open(ushort inode_id, ushort _uid, ushort _gid);
    std::pair<dir_entry, bool> read_dir_entry(ushort inode_id, ushort _uid, ushort _gid);
    size_t read(ushort inode_id, char* dst);
    std::pair<ushort, bool> new_file(std::string filename, ushort father_inode_id, ushort _owner, ushort _group, FILE_TYPE _filetype);
    std::pair<unsigned short, std::string> parse_path(std::string filepath, ushort _uid, ushort _gid);
    size_t _getsizeof(ushort inode_id);


};


#endif //VFS_INODE_H
