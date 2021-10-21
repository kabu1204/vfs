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
 *  接口和fstream相近
 */

class vfstream {
public:
    vfstream(inode_mgmt *_inode_manager, ushort _uid, ushort _gid);
    vfstream(inode_mgmt *_inode_manager, char path[128], char _mode, ushort _uid, ushort _gid);
    ~vfstream();
    size_t read(char *dst, size_t n);
    size_t write(char *src, size_t n);
    bool seekp(uint32 addr);
    bool seekg(uint32 addr);
    bool open(const std::string& path, char _mode);
    void close();
    bool is_open() const;
private:
    inode_mgmt* inode_manager;   // i节点管理器，用于向其申请读写权限，调用其接口
    char* _read_buffer;          // 只读缓冲区
    char* _write_buffer;         // 写缓冲区
    uint32 read_buffer_size;     // 只读缓冲区大小
    uint32 write_buffer_size;    // 写缓冲区大小
    uint32 written_size;         // 已写入的字节数
    char mode;                   // 模式: 'r'/'w'/'a'
    bool opened;                 // 状态
    size_t p;                    // 读写指针
    ushort uid;                  // 用户uid
    ushort gid;                  // 用户所属group id
    bool can_read;               // 读权限
    bool can_write;              // 写权限
    ushort inode_id;             // 当前读写文件的inode_id
};


#endif //VFS_VFSTREAM_H
