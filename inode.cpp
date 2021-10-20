#include "inode.h"
#include "superblock.h"
#include "dir_entry.h"
#include "util.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <utility>
#include <algorithm>
#include <cstdlib>
#include <string>

std::pair<ushort, bool> inode_mgmt::alloc_inode() {
    /*
     * 内部方法：
     *   申请一个i节点，并返回i节点号
     *   并不会对i节点信息进行设置，需要搭配其它方法，并被其它方法调用。
     */
    std::pair<ushort, bool> ret(0, false);
    if(spb->s_block.free_inode_num==0) return ret;

    ushort inode_id;
    if(!reclaimed_inodes.empty()){
        inode_id = reclaimed_inodes.back();
        reclaimed_inodes.pop_back();
        bitmap.set(inode_id);
        spb->s_block.free_inode_num -= 1;

        ret.first = inode_id;
        ret.second = true;
        return ret;
    }

    for(ushort i=last_p; i<MAX_INODE_NUM; ++i) {
        // 先从last_p处向后找
        if (!bitmap.test(i)) {
            bitmap.set(i);
            last_p = i;
            spb->s_block.free_inode_num -= 1;
            ret.first = i;
            ret.second = true;
            return ret;
        }
    }

    // 若last_p之后没有空闲i节点，则向前找
    for(ushort i=last_p-1; i>=0; --i) {
        if (!bitmap.test(i)) {
            bitmap.set(i);
            last_p = i;
            spb->s_block.free_inode_num -= 1;
            ret.first = i;
            ret.second = true;
            return ret;
        }
    }

    return ret;
}

std::pair<ushort, bool>
inode_mgmt::new_file(char _filename[128], ushort _owner, ushort _group, FILE_TYPE _filetype) {
    /*
     * 内部方法：
     *    创建新文件，这里的file包括普通文件和目录
     */
    std::pair<ushort, bool> ret(0, false);
    auto res = alloc_inode();
    ushort inode_id = res.first;
    ret.first = inode_id;
    ret.second = res.second;


    if(!res.second){
        std::cerr<<"Failed to allocate an inode!\n";
        return ret;
    }

    if(!load_inode(inode_id))
    {
        std::cerr<<"Failed to load inode into memory!\n";
        return ret;
    }

    inode& curr_inode = in_mem_inodes[inode_id];
    strcpy(curr_inode.fileName, _filename);
    curr_inode.owner = _owner;
    curr_inode.group = _group;
    curr_inode.fileType = _filetype;

    curr_inode.mtime = curr_inode.ctime = curr_inode.atime = time(nullptr);
    curr_inode.fileSize = 0;
    curr_inode.mode = 755;
    memset(curr_inode.blocks, 0, sizeof(curr_inode.blocks));
    auto ret2 = spb->alloc_block();
    if(ret2.second){
        curr_inode.blocks[0] = ret2.first;
        curr_inode.allocated_block_n = 1;
        curr_inode.fileSpace = BLOCK_SIZE;
        curr_inode.n_link = 1;
        curr_inode.allocated = true;
        ret.second = true;
    }
    else{
        std::cerr<<"Failed to allocate a block for new file!\n";
        reclaim_inode(inode_id);            // 分配block失败，释放刚刚申请的i节点
        ret.second = false;
    }

    return ret;
}

inode_mgmt::inode_mgmt(superblock_c *_spb, ioservice  *_io_context) : spb(_spb), io_context(_io_context) {
    /*
     * 构造函数：
     *   初始化时会扫描磁盘inode区域，将已分配的节点调入内存，并设置bitmap
     */
//    free_inode_num = _spb->s_block.free_inode_num;
    inode_start_addr = spb->s_block.inode_start_addr;
    inode_size = sizeof(inode);

    inode t;
    for(ushort i=0; i<MAX_INODE_NUM; ++i){
        io_context->read_s((char *)&t, inode_start_addr+i*inode_size, inode_size);
        if(t.allocated){
            in_mem_inodes[i] = t;   // 调入内存
            bitmap.set(i);          // 设置bitmap
            spb->s_block.free_inode_num -= 1;
            last_p = i;
        }
    }
}

bool inode_mgmt::load_inode(ushort inode_id) {
    /*
     * 接口：
     *   从磁盘读取inode到内存
     */
    if(!bitmap.test(inode_id)){
        std::cerr<<"inode not allocated!\n";
        return false;
    }

    inode t;
    io_context->read_s((char *)&t, inode_start_addr+inode_id*inode_size, inode_size);
    in_mem_inodes[inode_id] = t;

    assert(t.id == inode_id);
    return true;
}

bool inode_mgmt::store_inode(ushort inode_id) {
    /*
     * 接口：
     *   将内存中的inode写回磁盘，但不会释放内存中的inode
     */
    if(!bitmap.test(inode_id)){
        std::cerr<<"inode not allocated!\n";
        return false;
    }

    if(in_mem_inodes.count(inode_id)==0){
        std::cerr<<"inode not exist in memory!\n";
        return false;
    }

    io_context->read_s((char *)&(in_mem_inodes[inode_id]), inode_start_addr+inode_id*inode_size, inode_size);

    assert(in_mem_inodes[inode_id].id == inode_id);
    return true;
}

bool inode_mgmt::reclaim_inode(ushort inode_id) {
    /*
     * 内部方法：
     *   回收指定i节点，释放内存
     */
    if(!bitmap.test(inode_id)){
        std::cerr<<"inode not allocated!\n";
        return false;
    }

    if(in_mem_inodes.count(inode_id)==0){
        std::cerr<<"inode not exist in memory!\n";
        return false;
    }

    in_mem_inodes[inode_id].allocated = false;      // 状态置为未分配
    store_inode(inode_id);                          // 写回磁盘，否则下次初始化可能会读到错误状态

    in_mem_inodes.erase(inode_id);                  // 释放内存

    reclaimed_inodes.push_back(inode_id);           // 暂存被回收的inode号，以便下次快速分配
    bitmap.reset(inode_id);                         // 设置bitmap状态
    last_p = inode_id;
    spb->s_block.free_inode_num += 1;

    return true;
}

std::pair<ushort, bool>
inode_mgmt::mkdir(char *_filename, unsigned short prev_inode, unsigned short _owner, unsigned short _group) {
    /*
     * 接口：
     *   创建目录DIR
     *   _filename的合法性以及的权限的审查应在调用该接口前完成
     */
    std::pair<unsigned short, bool> ret(0, false);
    ushort inode_id;

    auto res = new_file(_filename, _owner, _group, DIR);     // 创建一个新文件，类型是DIR
    if(!res.second){
        std::cerr<<"Failed to create a new file!\n";
        ret.second = false;
        return ret;
    }
    inode_id = res.first;

    dir_entry new_dir(inode_id, prev_inode);      // 初始化新的dir

    if(!write_data(inode_id, (char*)&new_dir, sizeof(new_dir))){
        std::cerr<<"Failed to write dir_entry metadata to disk!\n";
        ret.second = false;
        return ret;
    }

    inode& curr_inode = in_mem_inodes[inode_id];
    curr_inode.fileSize = sizeof(new_dir);

    return ret;
}

std::pair<ushort, bool>
inode_mgmt::mkfile(char *_filename, unsigned short prev_inode, unsigned short _owner, unsigned short _group) {
    /*
     * 接口：
     *   创建普通文件NORMAL
     *   _filename的合法性以及的权限的审查应在调用该接口前完成
     */
    std::pair<unsigned short, bool> ret(0, false);

    auto res = new_file(_filename, _owner, _group, NORMAL);     // 创建一个新文件，类型是DIR
    if(!res.second){
        std::cerr<<"Failed to create a new file!\n";
        ret.second = false;
        return ret;
    }

    ret.first = res.first;
    ret.second = true;

    // 新文件暂无数据，直接返回i节点号
    return ret;
}

bool inode_mgmt::write_data(unsigned short inode_id, char *data, uint32 n) {
    ushort n_blocks = (n % BLOCK_SIZE == 0) ? n/BLOCK_SIZE : n/BLOCK_SIZE + 1;
    if(n_blocks > MAX_BLOCKS_PER_NODE){
        std::cerr<<"File size too big!\n";
        return false;
    }

    inode &curr_inode = in_mem_inodes[inode_id];

    if(curr_inode.allocated_block_n < n_blocks){
        // 需要申请更多的磁盘块以写入数据
        for(ushort i=curr_inode.allocated_block_n;i<n_blocks;++i){
            auto res = spb->alloc_block();  // 尝试申请新的磁盘块
            if(!res.second){
                // 申请失败
                std::cerr<<"Failed to allocate a block!\n";
                return false;
            }
            curr_inode.blocks[i] = res.first;
            curr_inode.allocated_block_n += 1;
        }
    }

    if(curr_inode.allocated_block_n > n_blocks){
        // 需要释放多余的磁盘块
        for(ushort i=n_blocks;i<curr_inode.allocated_block_n;++i){
            bool res = spb->reclaim_block(curr_inode.blocks[i]);
            if(!res){
                // 释放失败
                std::cerr<<"Failed to reclaim a block!\n";
                return false;
            }
            curr_inode.allocated_block_n -= 1;
        }
    }

    assert(curr_inode.allocated_block_n==n_blocks);

    uint32 block_id;
    uint32 start_addr;
    ushort size;
    for(ushort i=0; i < n_blocks; ++i){
        block_id = curr_inode.blocks[i];    // 获取当前写入的磁盘块号
        start_addr = block_id*BLOCK_SIZE;   // 块起始地址
        size = std::min<uint32>(BLOCK_SIZE, n-i*BLOCK_SIZE);    // 需要写入的字节数
        io_context->write_s(data+i*BLOCK_SIZE, start_addr, size);   // 写入
    }

    curr_inode.atime = time(nullptr);
    curr_inode.mtime = time(nullptr);

    return true;
}

std::pair<std::string, bool>
inode_mgmt::open(ushort inode_id, ushort _uid, ushort _gid) {
    /*
     * 内部方法：
     *   申请打开i节点所指向的文件(NORMAL类型)， 返回读写权限。
     *   供友元类vfstream调用
     */
    std::pair<std::string , bool> ret(0, false);

    inode& curr_inode = in_mem_inodes[inode_id];
    std::string right;
    if(_uid!=curr_inode.owner)
    {
        // 不是所有者
        if(curr_inode.group==_gid)
        {
            // 同group
            right = mode2str(curr_inode.mode)[1];
            ret.first = right;
            ret.second = true;
            return ret;
        }
        else{
            // 不同group
            right = mode2str(curr_inode.mode)[2];
        }
    }
    else{
        // 是所有者
        right = mode2str(curr_inode.mode)[0];
    }
    ret.first = right;
    ret.second = true;
    return ret;

}

std::pair<dir_entry, bool>
inode_mgmt::read_dir_entry(ushort inode_id, unsigned short _uid, unsigned short _gid) {
    /*
     * 接口：
     *  读取并返回inode_id对应的目录项
     *  会审查权限，具有读权限才可以获取目录项
     */
    dir_entry t(0, 0);
    std::pair<dir_entry, bool> ret(t, false);

    inode& curr_inode = in_mem_inodes[inode_id];

    if(curr_inode.fileType!=DIR){
        // inode_id指向的文件并不是目录项
        std::cerr<<curr_inode.fileName<<" is not a dir!\n";
        return ret;
    }

    auto res = open(inode_id, _uid, _gid);      // 返回读写权限
    if(!res.second){
        std::cerr<<"Failed to fetch rights!\n";
        return ret;
    }

    if(res.first[0]=='1'){
        // 具有读权限
        uint32 block_id = curr_inode.blocks[0];
        io_context->read_s((char *)&t, block_id*BLOCK_SIZE, sizeof(t));
        ret.first = t;
        ret.second = true;
    }
    else{
        std::cerr<<"Permission denied: you have no read access to the dir:"<<curr_inode.fileName<<"\n";
        ret.second = false;
    }

    return ret;
}

