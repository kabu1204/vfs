#ifndef VFS_VFSTREAM_H
#define VFS_VFSTREAM_H
#include <string>
#include <cstring>
#include "types.h"

/*
 * 类：
 *  用在该虚拟文件系统的文件输入输出流
 *  提供读取("r")、覆盖写入("w")、追加写入("a")
 */

class vfstream {
public:
    vfstream();
    vfstream(char path[MAX_FILENAME_LEN], std::string mode);
    size_t read(char *buffer, size_t n);
    size_t write(char *buffer, size_t n);
    bool open(char path[MAX_FILENAME_LEN], char mode);
    void close();
    bool is_open();
private:
    std::string _buffer;    // 缓冲区
    bool opened;            // 状态
    size_t p;               // 读写指针
};


#endif //VFS_VFSTREAM_H
