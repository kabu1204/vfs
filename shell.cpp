//
// Created by yuchengye on 2021/10/22.
//

#include "shell.h"


shell::shell(const char* _tty, int id):user(USER("ycy", 0, ROOT, ROOT)) {
    /*
     * 初始化：
     *   建立消息队列
     */
    logged = false;
    loop();
}

void shell::login(){
    /*
     * 登录
     */
    std::hash<std::string> hash_fn;
    ushort trials = 3;  // 最多尝试3次
    std::string username;
    std::string passwd;
    while(trials--){
        recv();
        std::cout<<msg.data;

        std::cin>>username;
        send(username.c_str());

        recv();
        std::cout<<msg.data;

        std::cin>>passwd;
        send(std::to_string(hash_fn(passwd)).c_str());

        recv();
        if(std::string(msg.data)=="login success"){
            send("ok");
            recv();
            memcpy((char*)&user, msg.data, sizeof(USER));
            recv();
            cwd = std::string(msg.data);
            send("ok");
            logged = true;
            return;
        }
    }
    std::cout<<"You wasted your 3 chances, please retry!\n";
}

shell::~shell() {
    msgctl(msq_id_send, IPC_RMID, nullptr);
}

void shell::loop() {

    std::string str;
    msq_id_send = msgget((key_t)1234, 0666|IPC_CREAT);
    if(msq_id_send==-1)
    {
        std::cerr<<"Message queue setup failed!\n";
        return;
    }

    long int msg_to_receive = 0;  //如果为0，表示消息队列中的所有消息都会被读取
    msq_id_recv = msgget((key_t)2345, 0666|IPC_CREAT);
    if(msq_id_recv==-1)
    {
        std::cerr<<"Message queue setup failed!\n";
        return;
    }

//    login();
//    if(!logged) return;

    prompt();
    while(std::getline(std::cin, str))
    {
        send(str.c_str());
        if(trimed(str)=="EXIT"){
            break;
        }


        recv();
        cwd = std::string (msg.data);
//        std::cout<<msg.data<<std::endl;
        prompt();
    }
}

void shell::prompt() {
    /*
     * 打印终端的提示信息
     */
    char prefix = (user.gid==ROOT) ? '#' : '$';     // #代表具有ROOT权限的用户   $代表普通用户
    //格式为 "prefix username in cwd"， 如 # root in /home/Document
    std::printf("\033[31m%c \033[1m%s\033[0m \033[36min\033[0m \033[1m\033[4m\033[37m%s\033[0m ", prefix, user.name, cwd.c_str());
}

void shell::send(const char* buffer) {
    msg.mtype = IPC_CLI2SERVER;
    strcpy(msg.data, buffer);
    if(msgsnd(msq_id_send, (void *)&msg, IPC_BUFFER_SIZE, 0)==-1){
        std::cerr<<"Failed to send message!"<<std::endl;
        return;
    }
}

void shell::recv() {
    long int msg_to_receive = 0;
    if(msgrcv(msq_id_recv, (void *)&msg, BUFSIZ, msg_to_receive, 0) == -1)
    {
        std::cerr<<"Failed to receive message!"<<std::endl;
        return;
    }
}
