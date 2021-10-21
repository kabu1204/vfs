//
// Created by yuchengye on 2021/10/22.
//

#ifndef VFS_MSQ_SERVICE_H
#define VFS_MSQ_SERVICE_H
#include <cstdio>
#include "sys/msg.h"
#include "sys/ipc.h"
#include "types.h"
#include "string"
#include "users.h"
#include "iostream"
#include "functional"
#include "util.h"
#include <ctime>
#include "sys/types.h"

class msq_service;
int exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb);

class msq_service {
public:
    msq_service(const char* _tty, int id, std::vector<USER> users, inode_mgmt *_inode_manager, superblock_c *_spb,
                std::string &_cwd);
    void loop(std::vector<USER> users);
    void login(std::vector<USER> users);
    void recv();
    void send(const char *buffer);
    ~msq_service();
private:
    inode_mgmt* inode_manager;
    superblock_c* spb;
    int msq_id_recv;
    int msq_id_send;
    bool logged;
    USER user;      // 当前shell登录的用户
    std::string& cwd;   // 当前工作目录
    msg_proto msg;   // 缓冲区
};


#endif //VFS_MSQ_SERVICE_H
