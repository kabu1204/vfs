//
// Created by yuchengye on 2021/10/18.
//

#include "inode.h"
#include "superblock.h"
#include <cstring>
#include <ctime>

inode::inode(char _filename[128], usigned _owner, usigned _group, FILE_TYPE _filetype) {
    strcpy(metaData.fileName, _filename);
    metaData.owner = _owner;
    metaData.group = _group;
    metaData.fileType = _filetype;

    metaData.mtime = metaData.ctime = metaData.atime = time(nullptr);
    metaData.fileSize = 0;
    metaData.mode = 755;
    pointers[0] =
    ulong fileSpace;                        // 文件所占空间

}
