//
// Created by yuchengye on 2021/10/19.
//

#ifndef VFS_IOSERVICE_H
#include <iostream>
#include <fstream>
#include "superblock.h"
#include "types.h"
#define VFS_IOSERVICE_H


class ioservice {
public:
    char disk_name[30];             // 磁盘名
    std::fstream handler;           // 负责读写磁盘
    uint32 inode_start_addr;         // inode区起始地址
    uint32 superblock_start_addr;    // superblock区起始地址
    uint32 root_dir_start_addr;      // 根目录区起始地址
    ushort inode_size;              // 每个inode的实际大小，单位Bytes

    friend class superblock_c;      // 声明superblock_c为友元类

    explicit ioservice(char name[30], uint32 _superblock_start_addr, uint32 _inode_start_addr, uint32 _root_dir_start_addr, ushort _inode_size);
    ~ioservice();
    bool read_s(char *buffer, std::streamsize start_addr, std::streamsize n);
    bool write_s(char *buffer, std::streamsize start_addr, std::streamsize n);

private:
    bool read(char *buffer, std::streamsize start_addr, std::streamsize n);
    bool write(char *buffer, std::streamsize start_addr, std::streamsize n);
};


#endif //VFS_IOSERVICE_H
