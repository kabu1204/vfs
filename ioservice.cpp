#include "ioservice.h"
#include "types.h"
#include <cstring>

ioservice::ioservice(char *name, uint32 _superblock_start_addr, uint32 _inode_start_addr, uint32 _root_dir_start_addr, ushort _inode_size):
    superblock_start_addr(_superblock_start_addr),
    inode_start_addr(_inode_start_addr),
    root_dir_start_addr(_root_dir_start_addr),
    inode_size(_inode_size)
    {
    /*
     * 初始化
     */
    strcpy(disk_name, name);
    handler.open(disk_name, std::ios::in | std::ios::out | std::ios::binary);
}

bool ioservice::read(char *buffer, std::streamsize start_addr, std::streamsize n) {
    /*
     * 内部方法：
     *   从start_addr处开始读取n字节数据到buffer中
     *   是不安全的，不会审查start_addr是否位于保留区中（superblock区）
     */
    if(start_addr<0 or start_addr+n>=MAX_SPACE or n<0) return false;

    handler.seekg(start_addr);
    handler.read(buffer, n);

    return true;
}

bool ioservice::read_s(char *buffer, std::streamsize start_addr, std::streamsize n) {
    /*
     * 接口：
     *   从start_addr处开始读取n字节数据到buffer中
     *   是安全的：
     *      1. 拒绝start_addr位于superblock区的访问
     *      2. 拒绝start_addr位于inode区、但没有对齐边界的访问
     *      3. 拒绝跨区域的访问
     */
    if(start_addr<0 or start_addr+n>=MAX_SPACE or n<0) return false;
    if(start_addr<inode_start_addr) return false;
    if(start_addr<root_dir_start_addr)
    {
        // 说明正在读取inode
        uint32 offset = start_addr - inode_start_addr;
        if((offset % inode_size) != 0) return false;    // 没有对齐
        if(n != inode_size) return false;               // inode读取一次一个
        if(offset+n > MAX_INODE_NUM*inode_size) return false;   // 越界
    }

    handler.seekg(start_addr);
    handler.read(buffer, n);

    return true;
}

bool ioservice::write(char *buffer, std::streamsize start_addr, std::streamsize n) {
    /*
     * 内部方法：
     *   从start_addr处开始将buffer的n字节数据写入到磁盘
     *   是不安全的，不会审查start_addr是否位于保留区中（superblock区）
     */
    if(start_addr<0 or start_addr+n>=MAX_SPACE or n<0) return false;

    handler.seekp(start_addr);
    handler.write(buffer, n);

    return true;
}

bool ioservice::write_s(char *buffer, std::streamsize start_addr, std::streamsize n) {
    /*
 * 接口：
 *   从start_addr处开始将buffer的n字节数据写入到磁盘
 *   是安全的：
 *      1. 拒绝start_addr位于superblock区的写入
 *      2. 拒绝start_addr位于inode区、但没有对齐边界的写入
 *      3. 拒绝跨区域的写入
 */
    if(start_addr<0 or start_addr+n>=MAX_SPACE or n<0) return false;
    if(start_addr<inode_start_addr) return false;
    if(start_addr<root_dir_start_addr)
    {
        // 说明正在写入inode
        uint32 offset = start_addr - inode_start_addr;
        if((offset % inode_size) != 0) return false;    // 没有对齐
        if(n != inode_size) return false;               // 一次只允许写入一个inode
        if(offset+n > MAX_INODE_NUM*inode_size) return false;   // 越界
    }

    handler.seekp(start_addr);
    handler.write(buffer, n);

    return true;
}

ioservice::~ioservice() {
    handler.close();
    std::cout<<"ioservice closed!\n";
}
