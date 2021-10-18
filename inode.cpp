//
// Created by yuchengye on 2021/10/18.
//

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


