#include "inode.h"
#include "superblock.h"
#include <cstring>
#include <ctime>
#include <utility>

inode::inode(char _filename[MAX_FILENAME_LEN], ushort _owner, ushort _group, FILE_TYPE _filetype, superblock_c* spb) {
    /*
     * 初始化一个新i节点，即创建新文件/目录
     */
    strcpy(metaData.fileName, _filename);
    metaData.owner = _owner;
    metaData.group = _group;
    metaData.fileType = _filetype;

    metaData.mtime = metaData.ctime = metaData.atime = time(nullptr);
    metaData.fileSize = 0;
    metaData.mode = 755;
    memset(metaData.blocks, 0, sizeof(metaData.blocks));
    auto ret = spb->alloc_block();
    if(ret.second){
        metaData.blocks[0] = ret.first;
        metaData.allocated_block_n = 1;
        metaData.fileSpace = BLOCK_SIZE;
        metaData.n_link = 1;
        metaData.allocated = true;
    }
    else{
        metaData.allocated_block_n = 0;
        metaData.fileSpace = 0;
        metaData.n_link = 0;
        metaData.allocated = false;
    }
}

bool inode::_del(superblock_c* spb) {
    /*
     * 内部方法：
     *   不经检查地 释放当前i节点及其所指向的磁盘空间，仅限内部调用
     */
    for(ushort i=0; i<metaData.allocated_block_n; ++i)
    {
        spb->reclaim_block(metaData.blocks[i]);
    }
    return true;
}

inode::~inode() {
    metaData.allocated = false;
}


std::pair<ushort, bool> inode_mgmt::alloc_inode() {
    /*
     * 内部方法：
     *   申请一个i节点，并返回i节点号
     *   并不会对i节点信息进行设置，需要搭配其它方法，并被其它方法调用。
     */
    std::pair<ushort, bool> ret(0, false);
    if(free_inode_num==0) return ret;

    ushort inode_id;
    if(!reclaimed_inodes.empty()){
        inode_id = reclaimed_inodes.back();
        reclaimed_inodes.pop_back();
        free_inode_num -= 1;

        ret.first = inode_id;
        ret.second = true;
        return ret;
    }

    for(ushort i=last_p; i<MAX_INODE_NUM; ++i) {
        // 先从last_p处向后找
        if (!bitmap.test(i)) {
            bitmap.set(i);
            last_p = i;
            free_inode_num -= 1;
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
            free_inode_num -= 1;
            ret.first = i;
            ret.second = true;
            return ret;
        }
    }


    return ret;
}
