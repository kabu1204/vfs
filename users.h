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
    char name[MAX_USER_NAME];   // 用户名
    ushort uid;                 // User ID
    ushort gid;                // Group ID
    USER(char _name[MAX_USER_NAME], ushort _uid=DEFAULT, ushort _gid=DEFAULT){
        uid = _uid;
        gid = _gid;
        strcpy(name, _name);
    }
};

#endif //VFS_USERS_H
