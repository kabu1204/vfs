//
// Created by yuchengye on 2021/10/20.
//

#include "vfstream.h"
#include <iostream>

vfstream::vfstream(char *path, std::string mode){


}

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
    switch (mode) {
        case 'r':


    }
    return false;
}

vfstream::vfstream() {
    /*
     * 初始化
     */
    opened = false;
}
