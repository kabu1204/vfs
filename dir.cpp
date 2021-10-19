#include "dir.h"

dir_metadata::dir_metadata(unsigned short curr_inode, unsigned short prev_inode){
    inode_arr[0] = curr_inode;
    inode_arr[1] = prev_inode;
}