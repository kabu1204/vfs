#ifndef VFS_SHELL_H
#define VFS_SHELL_H
#include <cstdio>
#include "sys/msg.h"
#include "sys/ipc.h"
#include "types.h"
#include "string"
#include "users.h"
#include "iostream"
#include "functional"
#include "util.h"

class shell {
public:
    shell(const char * tty, int id);
    void login();
    void recv();
    void send(const char* buffer);
    void prompt();
    void loop();
    ~shell();
private:
    char tty[64];   // 用于创建key的映射文件名
    key_t key;      // 用于创建消息队列的key
    int msq_id_send;     // 读队列
    int msq_id_recv;    //  写队列
    bool logged;
    USER user;      // 当前shell登录的用户
    msg_proto msg;   // 缓冲区
    std::string cwd="/";
};


#endif //VFS_SHELL_H
