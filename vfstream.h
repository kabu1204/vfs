#ifndef VFS_VFSTREAM_H
#define VFS_VFSTREAM_H
#include <string>
#include <cstring>
#include "types.h"
#include "inode.h"


/*
 * 类：
 *  用在该虚拟文件系统的文件输入输出流
 *  提供读取("r")、覆盖写入("w")、追加写入("a")
 */

class vfstream {
public:
    vfstream(inode_mgmt *_inode_manager, ushort _uid, ushort _gid);
    vfstream(inode_mgmt *_inode_manager, char path[128], char mode, ushort _uid, ushort _gid);
    size_t read(char *buffer, size_t n);
    size_t write(char *buffer, size_t n);
    bool open(char path[MAX_FILENAME_LEN], char mode);
    void close();
    bool is_open();
private:
    inode_mgmt* inode_manager;   // i节点管理器，用于向其申请读写权限，调用其接口
    std::string _buffer;         // 缓冲区
    bool opened;                 // 状态
    size_t p;                    // 读写指针
    ushort uid;                  // 用户uid
    ushort gid;                  // 用户所属group id
    bool can_read;               // 读权限
    bool can_write;              // 写权限
    ushort inode_id;             // 当前读写文件的inode_id
};


#endif //VFS_VFSTREAM_H
