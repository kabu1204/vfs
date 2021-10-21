//
// Created by yuchengye on 2021/10/19.
//

#ifndef VFS_IOSERVICE_H
#include <iostream>
#include <fstream>
#include "types.h"
#define VFS_IOSERVICE_H

class superblock_c;     // 因为需要声明superbloc_c是ioservice的友元类，因此在这声明类superbloc_c，定义在superbloc_c.h中

/*
 * 类：
 *   负责读写磁盘。作用类似真机环境的I/O系统调用。
 */
class ioservice {
public:
    friend class superblock_c;      // 声明superblock_c为友元类

    explicit ioservice(char name[30], uint32 _superblock_start_addr, uint32 _inode_start_addr, uint32 _root_dir_start_addr, ushort _inode_size);
    ~ioservice();
    bool read_s(char *buffer, std::streamsize start_addr, std::streamsize n);
    bool write_s(char *buffer, std::streamsize start_addr, std::streamsize n);

private:
    char disk_name[30];             // 磁盘名
    std::fstream handler;           // 负责读写磁盘
    uint32 inode_start_addr;         // inode区起始地址
    uint32 superblock_start_addr;    // superblock区起始地址
    uint32 root_dir_start_addr;      // 根目录区起始地址
    ushort inode_size;              // 每个inode的实际大小，单位Bytes

    bool read(char *buffer, std::streamsize start_addr, std::streamsize n);
    bool write(char *buffer, std::streamsize start_addr, std::streamsize n);
};


#endif //VFS_IOSERVICE_H
