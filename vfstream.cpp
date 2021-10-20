//
// Created by yuchengye on 2021/10/20.
//

#include "vfstream.h"
#include "inode.h"
#include "util.h"
#include <iostream>


bool vfstream::is_open() {
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
    opened = false;
    // TODO:写回
}

bool vfstream::open(char *path, char mode) {
    /*
     * 接口：
     *   打开文件
     */
    assert(mode=='r' || mode=='w' || mode=='a');
    if(is_open()){
        std::cerr<<"A file is already opened!\n";
        return false;
    }

    auto res = path2inode_id(path, inode_manager, uid, gid);
    if(!res.second){
        std::cerr<<"Failed to get path inode_id!\n";
        return false;
    }
    inode_id = res.first;

    if(inode_manager->in_mem_inodes[inode_id].fileType==DIR){
        std::cerr<<inode_manager->in_mem_inodes[res.first].fileName<<" is a directory!\n";
        return false;
    }

    auto res2 = inode_manager->open(res.first, uid, gid);
    if(!res2.second){
        std::cerr<<"Get permission failed!\n";
        return false;
    }

    switch (mode) {
        case 'r':   // 读模式
            // 检查读权限
            if(res2.first[0]=='0'){
                std::cerr<<"You do not have the permission to read!\n";
                return false;
            }
            else can_read = true;
            break;
        default:    // 覆盖写模式或追加模式
            // 检查写权限
            if(res2.first[1]=='0'){
                std::cerr<<"You do not have the permission to read!\n";
                return false;
            }
            else can_write = true;
            break;
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
}

vfstream::vfstream(inode_mgmt *_inode_manager, char *path, char mode, unsigned short _uid, unsigned short _gid) {
    inode_manager = _inode_manager;
    uid = _uid;
    gid = _gid;
    can_read = false;
    can_write = false;
    open(path, mode);
}
