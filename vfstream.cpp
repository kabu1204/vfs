#include "vfstream.h"
#include "util.h"
#include <iostream>


bool vfstream::is_open() const {
    /*
     * 接口：
     *   返回文件是否已打开
     */
    return opened;
}

void vfstream::close() {
    /*
     * 接口：
     *   关闭文件
     */
    can_read = false;
    can_write = false;
    opened = false;
    if(mode=='r'){
        // 读模式
        delete _read_buffer;
    }
    else if(mode=='w'){
        // 覆盖写模式，直接写回磁盘
        if(!inode_manager->write_data(inode_id, _write_buffer, written_size)){
            std::cerr<<"Failed to write back to disk!\n";
        }
        delete _write_buffer;
    }
    else{
        // 追加写模式，先合并，再写回磁盘
        char *concat = new char[read_buffer_size+written_size];     // 开辟一个新缓冲区
        memcpy(concat, _read_buffer, read_buffer_size);         // 将只读缓冲区内容拷贝到concat中
        memcpy(concat+read_buffer_size, _write_buffer, written_size);   // 再将追加内容拷贝到concat后半段中
        if(!inode_manager->write_data(inode_id, concat, written_size)){
            std::cerr<<"Failed to write back to disk!\n";
        }
        delete _read_buffer;
        delete _write_buffer;
    }
}

bool vfstream::open(const std::string& path, char _mode) {
    /*
     * 接口：
     *   打开文件
     */
    mode = _mode;
    assert(_mode == 'r' || _mode == 'w' || _mode == 'a');
    if(is_open()){
        std::cerr<<"A file is already opened!\n";
        return false;
    }

    auto res = inode_manager->get_inode_id(path, uid, gid);
    if(!res.second){
        if(mode=='w') {
            // 覆盖写模式，不存在则尝试创建
            auto res2 = inode_manager->mkfile(path, uid, gid);  // 尝试创建path
            if(!res2.second){
                std::cerr<<"No such file or directory: "<<path<<"\n";       // path非法或者不存在
                return false;   // 说明path的上级目录也非法或不存在
            }
        }
        else{
            // 读模式和追加模式需要文件存在，因此直接返回错误
            std::cerr<<"No such file or directory: "<<path<<"\n";       // path非法或者不存在
            return false;
        }
    }
    inode_id = res.first;

    if(inode_manager->in_mem_inodes[inode_id].fileType==DIR){
        std::cerr<<inode_manager->in_mem_inodes[inode_id].fileName<<" is a directory!\n";
        return false;
    }

    auto res2 = inode_manager->open(inode_id, uid, gid);
    if(!res2.second){
        std::cerr<<"Get permission failed!\n";
        return false;
    }


    if(_mode == 'r') { // 读模式
        // 检查读权限
        if (res2.first[0] == '0') {
            std::cerr << "You do not have the permission to read!\n";
            return false;
        } else {
            can_read = true;        // 设置为可读
            can_write = false;      // 设置为不可写
            read_buffer_size = inode_manager->in_mem_inodes[inode_id].fileSize;  // 只读缓冲区大小等于文件大小
            _read_buffer = new char[read_buffer_size];               // 开辟只读缓冲区的空间
            inode_manager->read(inode_id, _read_buffer);     // 把i节点对应的磁盘数据读到缓冲区
            p = 0;  // 设置指针位置为起始位置
        }
    }
    else if(_mode == 'w'){     // 覆盖写模式
        // 检查写权限
        if (res2.first[1] == '0') {
            std::cerr << "You do not have the permission to write!\n";
            return false;
        } else {
            can_write = true;   // 设置为可写
            can_read = false;   // 因为覆盖写模式下新开辟的缓冲区是空的，所以禁止读。
            _write_buffer = new char[BLOCK_SIZE];   // 写缓冲区大小默认为一个块大小，可以增长
            written_size = 0;   // 已写入缓冲区0字节
        }
    }
    else{    // 追加写模式
        // 检查写权限
        if (res2.first[1] == '0') {
            std::cerr << "You do not have the permission to write!\n";
            return false;
        } else {
            can_write = true;   // 设置为可写
            can_read = false;   // 写缓冲区是追加内容，初始也是空的，禁止读

            // 同时开辟读和写缓冲区，读缓冲区存原先内容，写缓冲区存追加内容，close时拼接并写回文件
            read_buffer_size = inode_manager->in_mem_inodes[inode_id].fileSize;  // 只读缓冲区大小等于文件大小
            _read_buffer = new char[read_buffer_size];               // 开辟只读缓冲区的空间
            inode_manager->read(inode_id, _read_buffer);     // 把i节点对应的磁盘数据读到只读缓冲区
            _write_buffer = new char[BLOCK_SIZE];           // 写缓冲区大小默认为一个块大小，可以增长
            written_size = 0;   // 已写入缓冲区0字节
        }
    }

    opened = true;
    return true;
}

vfstream::vfstream(inode_mgmt *_inode_manager, ushort _uid, ushort _gid){
    /*
     * 初始化
     */
    inode_manager = _inode_manager;
    uid = _uid;
    gid = _gid;
    opened = false;
    can_read = false;
    can_write = false;
    _read_buffer = nullptr;
    _write_buffer = nullptr;
    read_buffer_size = 0;
    write_buffer_size = 0;
    written_size = 0;
}

vfstream::vfstream(inode_mgmt *_inode_manager, std::string path, char _mode, unsigned short _uid, unsigned short _gid) {
    /*
     * 初始化并打开
     */
    inode_manager = _inode_manager;
    uid = _uid;
    gid = _gid;
    opened = false;
    can_read = false;
    can_write = false;
    _read_buffer = nullptr;
    _write_buffer = nullptr;
    read_buffer_size = 0;
    write_buffer_size = 0;
    written_size = 0;
    open(path, _mode);
}

size_t vfstream::read(char *dst, size_t n) {
    /*
     * 接口：
     *   从读指针p处开始读取最多n个字节到dst中
     *   会对n进行合法审查，返回实际读取到的字节数。
     */
    if(!can_read) return -1;
    if(n<0) return -1;

    uint32 size = std::min<uint32>(read_buffer_size - p, n);
    memcpy(dst, _read_buffer + p, size);
    p = p + size;

    return size;
}

size_t vfstream::write(char *src, size_t n) {
    /*
     * 接口：
     *   将src开始的n个字节 从写指针p处，开始写入到缓冲区_buffer
     *   会对n进行合法审查，返回实际写入的字节数。
     */
    if(!can_write) return -1;
    if(n<0) return -1;

    if(p+n>write_buffer_size){
        // 写缓冲区大小不够，开辟新空间
        uint32 extra_size = ((p+n-write_buffer_size)/BLOCK_SIZE + 1)*BLOCK_SIZE;
        char *tmp = new char[write_buffer_size + extra_size];   // 申请新空间
        memcpy(tmp, _write_buffer, written_size);   // 把写缓冲区的内容拷贝到新空间
        delete _write_buffer;       // 释放旧空间的内存
        _write_buffer = tmp;        // 指向新空间
        write_buffer_size += extra_size;
    }

    memcpy(_write_buffer+p, src, n);
    written_size += n;
    p = p + n;

    return n;
}

bool vfstream::seekp(uint32_t addr) {
    /*
     * 设置写指针位置，因为一个vfstream对象同一时间只能读或只能写，因此指针是共享的。
     */
    if(!can_write) return false;
    if(addr<0 or addr >= read_buffer_size) return false;
    p = addr;
}

bool vfstream::seekg(uint32_t addr) {
    /*
     * 设置读指针位置，因为一个vfstream对象同一时间只能读或只能写，因此指针是共享的。
     */
    if(!can_read) return false;
    if(addr<0 or addr >= read_buffer_size) return false;
    p = addr;
}

vfstream::~vfstream() {
    /*
     * 析构函数
     */
    close();
}

bool vfstream::eof() {
    /*
     * 返回是否已读到文件结束
     */
    if(!opened) return false;
    if(!can_read) return false;
    return p==read_buffer_size;
}
