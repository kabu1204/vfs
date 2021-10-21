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
inode_mgmt::new_file(std::string filename, ushort father_inode_id, ushort _owner, ushort _group, FILE_TYPE _filetype) {
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
        if(!reclaim_inode(inode_id)){   // 加载inode失败，退回刚刚分配的inode
            std::cerr<<"Failed to reclaim inode!\n";
        }
        return ret;
    }

    // 设置i节点属性
    inode& curr_inode = in_mem_inodes[inode_id];
    strcpy(curr_inode.fileName, filename.c_str());
    curr_inode.owner = _owner;
    curr_inode.group = _group;
    curr_inode.fileType = _filetype;

    curr_inode.mtime = curr_inode.ctime = curr_inode.atime = time(nullptr);
    curr_inode.fileSize = 0;
    curr_inode.mode = 755;
    memset(curr_inode.blocks, 0, sizeof(curr_inode.blocks));
    auto res2 = spb->alloc_block();
    if(res2.second){
        curr_inode.blocks[0] = res2.first;
        curr_inode.allocated_block_n = 1;
        curr_inode.fileSpace = BLOCK_SIZE;
        curr_inode.n_link = 1;
        curr_inode.allocated = true;
        ret.second = true;
    }
    else{
        std::cerr<<"Failed to allocate a block for new file!\n";
        if(!reclaim_inode(inode_id)){   // 分配block失败，退回刚刚分配的inode
            std::cerr<<"Failed to reclaim inode!\n";
        }
        ret.second = false;
    }

    if(ret.second and filename!="/"){
        // 父目录的fileNum+1，并且其inode_arr需要添加新文件的i节点号
        auto res3 = read_dir_entry(father_inode_id, _owner, _group);
        if(!res3.second){
            std::cerr<<"Failed to fetch father directory\'s entry!\n";
            if(!reclaim_inode(inode_id)){   // 退回刚刚分配的inode
                std::cerr<<"Failed to reclaim inode!\n";
            }
            ret.second = false;
            return ret;
        }
        dir_entry father_entry = res3.first;
        auto res4 = get_child_file(father_entry,this);  // 获取父目录下的文件，检查是否重复创建
        if(!res4.second){
            std::cerr<<"Failed to get child file list!\n";
            if(!reclaim_inode(inode_id)){   // 退回刚刚分配的inode
                std::cerr<<"Failed to reclaim inode!\n";
            }
            ret.second = false;
            return ret;
        }
        for(auto name:res4.first){
            if(filename==name){     // 要创建的文件或目录已存在
                std::cerr<<(_filetype==NORMAL?"File":"Directory")<<" already exist!\n";
                if(!reclaim_inode(inode_id)){   // 退回刚刚分配的inode
                    std::cerr<<"Failed to reclaim inode!\n";
                }
                ret.second = false;
                return ret;
            }
        }
        // 更新父目录项的属性
        father_entry.inode_arr[father_entry.fileNum + 2] = inode_id;
        father_entry.fileNum += 1;
        // 更新父i节点的属性
        in_mem_inodes[father_inode_id].atime = time(nullptr);
        in_mem_inodes[father_inode_id].mtime = time(nullptr);
        // 将更新后的父目录项写回磁盘
        if(!write_data(father_inode_id, (char *)&father_entry, sizeof(father_entry))){
            std::cerr<<"Failed to write updated father directory entry to disk!\n";
            if(!reclaim_inode(inode_id)){   // 退回刚刚分配的inode
                std::cerr<<"Failed to reclaim inode!\n";
            }
            ret.second = false;
            return ret;
        }
    }
    ret.first = inode_id;
    ret.second = true;

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
    last_p = 0;

    inode t;
    for(ushort i=0; i<MAX_INODE_NUM; ++i){
        io_context->read_s((char *)&t, inode_start_addr+i*inode_size, inode_size);
        if(t.allocated){
            in_mem_inodes[i] = t;   // 调入内存
            bitmap.set(i);          // 设置bitmap
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

    io_context->write_s((char *)&(in_mem_inodes[inode_id]), inode_start_addr+inode_id*inode_size, inode_size);

    assert(in_mem_inodes[inode_id].id == inode_id);
    return true;
}

bool inode_mgmt::reclaim_inode(ushort inode_id) {
    /*
     * 内部方法：
     *   回收指定i节点, 释放内存，并且释放所占有的磁盘块
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
    for(uint32 )

    in_mem_inodes.erase(inode_id);                  // 释放内存

    reclaimed_inodes.push_back(inode_id);           // 暂存被回收的inode号，以便下次快速分配
    bitmap.reset(inode_id);                         // 设置bitmap状态
    last_p = inode_id;

    return true;
}

std::pair<ushort, bool>
inode_mgmt::mkdir(std::string _filepath, unsigned short _owner, unsigned short _group) {
    /*
     * 接口：
     *   创建目录DIR
     *   _filename的合法性以及的权限的审查应在调用该接口前完成
     */
    std::pair<ushort, bool> ret(0, false);
    ushort inode_id;
    std::string filename;
    ushort father_inode_id = 0;

    auto parsed = parse_path(_filepath, _owner, _group);
    if(parsed.second.empty()){
        std::cerr<<"Failed to parse path: "<<_filepath<<", please check it!\n";
        return ret;
    }

    father_inode_id = parsed.first;
    filename = parsed.second;

    auto res = new_file(filename, father_inode_id, _owner, _group, DIR);     // 创建一个新文件，类型是DIR
    if(!res.second){
        std::cerr<<"Failed to create a new file!\n";
        ret.second = false;
        return ret;
    }
    inode_id = res.first;

    dir_entry new_dir(inode_id, father_inode_id);      // 初始化新的目录项

    if(!write_data(inode_id, (char*)&new_dir, sizeof(new_dir))){
        std::cerr<<"Failed to write dir_entry metadata to disk!\n";
        ret.second = false;
        return ret;
    }

    inode& curr_inode = in_mem_inodes[inode_id];
    curr_inode.fileSize = sizeof(new_dir);
    ret.first = inode_id;
    ret.second = true;

    return ret;
}

std::pair<ushort, bool>
inode_mgmt::mkfile(std::string _filepath, unsigned short _owner, unsigned short _group) {
    /*
     * 接口：
     *   创建普通文件NORMAL
     *   _filename的合法性以及的权限的审查应在调用该接口前完成
     */
    std::pair<unsigned short, bool> ret(0, false);
    std::string filename;
    ushort father_inode_id = 0;

    auto parsed = parse_path(_filepath, _owner, _group);
    if(parsed.second.empty()){
        std::cerr<<"Failed to parse path: "<<_filepath<<", please check it!\n";
        return ret;
    }

    father_inode_id = parsed.first;
    filename = parsed.second;

    auto res = new_file(filename, father_inode_id, _owner, _group, NORMAL);     // 创建一个新文件，类型是NORMAL
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
    /*
     * 内部方法：
     *   将data写入inode_id对应的磁盘块中
     */
    ushort n_blocks = (n % BLOCK_SIZE == 0) ? n/BLOCK_SIZE : n/BLOCK_SIZE + 1;
    if(n_blocks > MAX_BLOCKS_PER_NODE){
        std::cerr<<"File size too big!\n";
        return false;
    }

    inode &curr_inode = in_mem_inodes[inode_id];

    if(curr_inode.allocated_block_n < n_blocks){
        // 需要申请更多的磁盘块以写入数据
        uint32 diff = n_blocks - curr_inode.allocated_block_n;  // 还需申请diff个磁盘块
        auto res = spb->alloc_n_blocks(n_blocks - curr_inode.allocated_block_n);
        if(!res.second){
            // 申请失败
            std::cerr<<"Failed to allocate "<<diff<<" blocks!\n";
            return false;
        }
        for(uint32 block_id:res.first){
            curr_inode.blocks[curr_inode.allocated_block_n] = block_id;
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

    // 更新i节点信息
    curr_inode.fileSize = n;
    curr_inode.fileSpace = n_blocks*BLOCK_SIZE;
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
    std::pair<std::string , bool> ret("", false);

    inode& curr_inode = in_mem_inodes[inode_id];
    std::string mode_bin = hex2bin(curr_inode.mode);   // 八进制数转二进制字符串
    if(_uid!=curr_inode.owner)
    {
        // 不是所有者
        if(curr_inode.group==_gid)
        {
            // 同group
            ret.first = mode_bin.substr(3, 3);
            ret.second = true;
            return ret;
        }
        else{
            // 不同group
            mode_bin = mode_bin.substr(6, 3);
        }
    }
    else{
        // 是所有者
        mode_bin = mode_bin.substr(0, 3);
    }
    curr_inode.atime = time(nullptr);   // 更新i节点的上次打开时间
    ret.first = mode_bin;
    ret.second = true;
    return ret;

}

std::pair<dir_entry, bool>
inode_mgmt::read_dir_entry(ushort inode_id, unsigned short _uid, unsigned short _gid) {
    /*
     * 内部方法：
     *  读取并返回inode_id对应的目录项
     *  会审查权限，具有读权限才可以获取目录项
     */
    dir_entry t(0, 0);
    std::pair<dir_entry, bool> ret(t, false);

    if(!isAllocated(inode_id)) return ret;  // i节点未分配

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


/*
 * 内部方法：
 *   读取inode_id对应的磁盘块的所有数据到dst中。
 *   供vfstream友元类调用
 *   返回读取的字节数
 */
size_t inode_mgmt::read(ushort inode_id, char* dst) {
    /*
     * 内部方法：
     *   读取inode_id对应的磁盘块的所有数据到dst中。
     *   供vfstream友元类调用
     *   返回读取的字节数
     */
    inode &curr_inode = in_mem_inodes[inode_id];
    size_t n = curr_inode.fileSize;

    uint32 block_id;
    uint32 start_addr;
    ushort size;
    for(ushort i=0;i<curr_inode.allocated_block_n;++i){
        block_id = curr_inode.blocks[i];        // 当前读取的block
        size = std::min<uint32>(BLOCK_SIZE, n-i*BLOCK_SIZE);     // 当前读取的字节数
        start_addr = block_id*BLOCK_SIZE;        // 当前读取的block的起始地址
        io_context->read_s(dst+i*BLOCK_SIZE, start_addr, size);
    }

    return n;
}

std::pair<ushort, std::string> inode_mgmt::parse_path(std::string filepath, ushort _uid, ushort _gid) {
    /*
     * 内部方法：
     *   解析filepath，返回<父目录路径, filepath所指路径的文件名/目录名>
     *   如filepath="/dir1/1.txt"时，返回<"/dir1", "1.txt">
     */
    std::pair<ushort, std::string> ret(0, "");
    std::string filename;
    std::string father_dir;
    ushort father_inode_id = 0;
    auto split_path = split(filepath,"/");

    if(split_path.empty()){
        filename = "/";
        father_dir = "";
    }
    else if(filepath.back()=='/'){
        filename = split_path.back();
        father_dir = filepath.erase(filepath.length()-filename.length()-1);   // 获取filename所在目录，即父目录
    }
    else{
        filename = split_path.back();
        father_dir = filepath.erase(filepath.length()-filename.length());     // 获取filename所在目录，即父目录
    }

    if(filename.length()>MAX_FILENAME_LEN){
        std::cerr<<"Filename too long! (Limit: 128)\n";
        return ret;
    }

    if(filename!="/"){
        // 获取父目录对应的i节点
        auto res = path2inode_id(father_dir, this, _uid, _gid, 0);
        if(!res.second){
            std::cerr<<"Failed to get inode for the path: "<<father_dir<<"\n";
            return ret;
        }
        father_inode_id = res.first;
    }

    ret.first = father_inode_id;
    ret.second = filename;

    return ret;
}

size_t inode_mgmt::getsizeof(std::string _filepath, ushort _uid, ushort _gid) {
    /*
     * 接口：
     *   获取文件的大小(Bytes)
     */
    auto res = path2inode_id(_filepath, this, _uid, _gid, 0);
    if(!res.second){
        std::cerr<<"Failed to get "<<_filepath<<"\'s inode_id!\n";
        return -1;
    }
    return getsizeof(res.first);
}

size_t inode_mgmt::getsizeof(ushort inode_id) {
    /*
     * 接口：
     *   获取指定i节点号所指向文件的大小
     *   i节点未分配时返回-1
     */
    if(!isAllocated(inode_id))  return -1;  // i节点未分配
    return in_mem_inodes[inode_id].fileSize;
}

inode_mgmt::~inode_mgmt() {
    /*
     * 析构函数：
     *   将所有已读入内存的i节点全部写回磁盘
     */
    for(auto pair:in_mem_inodes){
        std::cout<<pair.second.allocated<<"\n";
        store_inode(pair.first);
    }
    std::cout<<"Successfully wrote inodes back to disk!\n";
}

std::pair<std::string, bool>
inode_mgmt::get_protection_code(unsigned short inode_id) {
    /*
     * 接口：
     *   获取文件保护码
     */
    std::pair<std::string, bool> ret("", false);
    std::string code = "-";

    if(!isAllocated(inode_id)){
        std::cerr<<"File not exists!\n";
        return ret;
    }
    inode& curr_inode = in_mem_inodes[inode_id];

    if(curr_inode.fileType==DIR)
    {
        // 该文件是目录类型
        code[0] = 'd';
    }

    code = code + mode2str(curr_inode.mode);
    ret.first = code;
    ret.second = true;

    return ret;
}

bool inode_mgmt::isDir(unsigned short inode_id) {
    /*
     * 接口：
     *  判断给定的i节点是否指向一个目录项
     */
    if(!isAllocated(inode_id))  return false;   // i节点未分配

    return in_mem_inodes[inode_id].fileType==DIR;
}

std::pair<dir_entry, bool> inode_mgmt::get_dir_entry(unsigned short inode_id, ushort _uid, ushort _gid) {
    /*
     * 接口：
     *   获取给定i节点指向的目录项
     */
    std::pair<dir_entry, bool> ret(dir_entry(), false);
    if(!isDir(inode_id)){
        std::cerr<<"The inode does not point to a dir!\n";
        return ret;
    }

    return read_dir_entry(inode_id, _uid, _gid);
}

std::pair<uint32, bool> inode_mgmt::get_addr_of(ushort inode_id) {
    /*
     * 接口：
     *  获取给定i节点指向文件的起始地址
     */
    std::pair<uint32_t, bool> ret(0, false);
    if(!isAllocated(inode_id)) return ret;      // i节点未分配

    ret.first = in_mem_inodes[inode_id].blocks[0]*BLOCK_SIZE;
    ret.second = true;
    return ret;
}

bool inode_mgmt::isAllocated(unsigned short inode_id) {
    /*
     * 接口：
     *  判断给定i节点号是否已分配
     */
    if(!bitmap.test(inode_id)){
        std::cerr<<"inode not allocated!\n";
        return false;
    }
    return true;
}

std::pair<std::string, bool> inode_mgmt::get_name_of(unsigned short inode_id) {
    /*
     * 接口：
     *  返回给定i节点号对应的文件名
     */
    std::pair<std::string, bool> ret("", false);
    if(!isAllocated(inode_id))  return ret;     // i节点未分配
    ret.first = in_mem_inodes[inode_id].fileName;
    ret.second = true;
    return ret;
}

std::pair<std::string, bool> inode_mgmt::get_full_path_of(ushort inode_id, ushort _uid, ushort _gid) {
    /*
     * 接口：
     *   或者指定i节点对应文件的全路径（绝对路径）
     */
    std::pair<std::string, bool> ret("", false);
    auto res = get_name_of(inode_id);
    if(!res.second) return ret;     // 获取文件名失败
    std::string path = res.first;

    ushort curr_inode_id = inode_id;
    while(curr_inode_id!=0){
        auto res2 = get_dir_entry(inode_id, _uid, _gid);
        if(!res2.second) return ret;    // 获取目录项失败
        curr_inode_id = res2.first.inode_arr[1];
        res = get_name_of(curr_inode_id);
        if(!res.second) return ret;     // 获取目录名失败
        path = get_name_of(curr_inode_id).first + "/" + path;
    }
    ret.first = format_path(path);
    ret.second = true;
    return ret;
}

bool
inode_mgmt::rm_file(std::string filename, ushort father_inode_id, ushort _uid, ushort _gid) {
    /*
     * 内部方法：
     *   删除father_inode_id对应的目录下一个名为filename的文件，非递归方式。如果文件是非空的目录，则不会删除
     */
    auto res = read_dir_entry(father_inode_id, _uid, _gid);
    if(!res.second) return false;   // 获取目录项失败，删除失败
    dir_entry father_entry = res.first;

    for(ushort i=0;i<father_entry.fileNum;++i){
        ushort inode_id = father_entry.inode_arr[i+2];
        auto res2 = get_name_of(inode_id);
        if(!res2.second) return false;  // 获取文件名失败
        if(res2.first == filename){
            if(in_mem_inodes[inode_id].fileType==DIR)
            {
                res = read_dir_entry(inode_id, _uid, _gid);
                if(!res.second) return false;   // 获取目录项失败
                if(res.first.fileNum!=0) return false;  // 非空目录，删除失败
            }
        }
    }

    return false;
}

