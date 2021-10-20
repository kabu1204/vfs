#ifndef VFS_UTIL_H
#define VFS_UTIL_H
#include "string"
#include "vector"
#include "utility"
#include "types.h"
#include "inode.h"
#include "dir_entry.h"

std::string& trim(std::string &str);

std::vector<std::string> split(const std::string& str, const std::string& delim);

std::string mode_num2bin(ushort mode);

std::vector<std::string> mode2str(ushort mode);

std::pair<std::vector<std::string>, bool> get_child_file(dir_entry entry, inode_mgmt* inode_manager);

std::pair<ushort, bool> path2inode_id(char *path, inode_mgmt* inode_manager, ushort _uid, ushort _gid);

#endif //VFS_UTIL_H
