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
    int msq_id_send;        // 发送队列
    int msq_id_recv;        //  接收队列
    bool logged;            // 登录状态
    USER user;              // 当前shell登录的用户
    msg_proto msg;          // 缓冲区
    std::string cwd="/";    // 当前工作目录
};


#endif //VFS_SHELL_H
