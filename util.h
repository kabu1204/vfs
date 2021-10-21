#ifndef VFS_UTIL_H
#define VFS_UTIL_H
#include "string"
#include "vector"
#include "utility"
#include "types.h"
#include "inode.h"
#include "dir_entry.h"

#define HASH_STRING_PIECE(string_piece)                                       \
    size_t result = 0;                                                        \
    for (auto it = string_piece.cbegin(); it != string_piece.cend(); ++it) {  \
        result = (result * 131) + *it;                                        \
    }                                                                         \
    return result;

std::string& trim(std::string &str);

std::string trimed(std::string str);

std::vector<std::string> split(const std::string& str, const std::string& delim);

std::string mode_num2bin(ushort mode);

std::vector<std::string> mode2str(ushort mode);

std::string hex2bin(uint32 n);

std::string _hex2bin(ushort n);

ushort str2gid(const std::string& str);

std::pair<std::vector<std::string>, bool> get_child_file(dir_entry entry, inode_mgmt* inode_manager);

std::string get_protection_code(inode_mgmt* inode_manager);

std::pair<ushort, bool> path2inode_id(const std::string& path, inode_mgmt* inode_manager, ushort _uid, ushort _gid);

std::pair<std::string, bool> concat_path(std::string a, std::string b);

#endif //VFS_UTIL_H
