//
// Created by yuchengye on 2021/10/22.
//

#ifndef VFS_INIT_H
#define VFS_INIT_H
#include "inode.h"
#include "ioservice.h"
#include "types.h"
#include "superblock.h"
#include "users.h"
#include "util.h"
#include "cstdio"
#include "iostream"
#include "fstream"
#include "vfstream.h"


int Load(superblock_c *&spb, ioservice *&io_context, inode_mgmt *&inode_manager);

int Format(inode_mgmt *&inode_manager, ioservice *&io_context, superblock_c *&spb);

void format_inodeSpace(std::ofstream *disk, superblock_c *spb);

void format_Superblock(std::ofstream *disk, superblock_c* &spb);

int format_rootDir(superblock_c *spb, ioservice *io_context, inode_mgmt* &inode_manager);

int init_DefaultUsers(superblock_c *spb, ioservice *io_context, inode_mgmt  *inode_manager);

#endif //VFS_INIT_H
