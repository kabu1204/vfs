#ifndef VFS_USERS_H
#define VFS_USERS_H

#include "types.h"
#include <string>

// 以下是特殊的UID/GID
#define LOCAL 0
#define SYSTEM 1
#define ROOT 2
#define DEFAULT 3

struct USER{
    char name[MAX_USER_NAME_LEN];   // 用户名
    size_t passwd;                  // 用户密码，经过hash后的
    ushort uid;                 // User ID
    ushort gid;                // Group ID
    USER(char _name[64], size_t _passwd, unsigned short _uid = DEFAULT, unsigned short _gid = DEFAULT) {
        passwd = _passwd;
        uid = _uid;
        gid = _gid;
        strcpy(name, _name);
    }
    USER(const std::string &_name, size_t _passwd, unsigned short _uid = DEFAULT, unsigned short _gid = DEFAULT) {
        passwd = _passwd;
        uid = _uid;
        gid = _gid;
        strcpy(name, _name.c_str());
    }
};

#endif //VFS_USERS_H
