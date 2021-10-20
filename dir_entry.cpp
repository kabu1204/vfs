#include "dir_entry.h"

dir_entry::dir_entry(ushort curr_inode, ushort prev_inode){
    inode_arr[0] = curr_inode;
    inode_arr[1] = prev_inode;
}