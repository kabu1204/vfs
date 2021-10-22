//
// Created by yuchengye on 2021/10/22.
//

#include "msq_service.h"
msq_service::msq_service(const char* _tty, int id, std::vector<USER> users, inode_mgmt *_inode_manager, superblock_c *_spb,
                         std::string &_cwd)
        : user(USER("ycy", 0, ROOT, ROOT)), cwd(_cwd) {
    /*
     * 初始化：
     *   建立消息队列
     */
    logged = false;
    inode_manager = _inode_manager;
    spb = _spb;
    loop(users);
}

void msq_service::loop(std::vector<USER> users) {
    /*
     *  主循环
     *  服务端是读-写-读-写交替进行
     */

    long int msg_to_receive = 0;  //如果为0，表示消息队列中的所有消息都会被读取
    time_t t = time(nullptr);
    char cur_time[64];

    // 创建消息队列
    // 这是读队列
    msq_id_recv = msgget((key_t) 1234, 0666 | IPC_CREAT);
    if (msq_id_recv == -1) {
        std::cerr << "Message queue setup failed!" << std::endl;
        return;
    }

    // 这是写队列
    msq_id_send = msgget((key_t) 2345, 0666 | IPC_CREAT);
    if (msq_id_send == -1) {
        std::cerr << "Message queue setup failed!" << std::endl;
        return;
    }

//    login(users);
//    if(!logged) return;

    //接收消息队列中的消息直到遇到一个end消息。最后，消息队列被删除。
    while(true) {
        t = time(nullptr);
        strftime(cur_time, sizeof(cur_time), "[%Y-%m-%d %H:%M:%S]", localtime(&t));
        std::cout << std::endl << cur_time << std::flush;

        //第5个参数为0表示阻塞方式，当消息队列为空，一直等待
        recv();
        printf("%s\n", msg.data);

        std::string str(msg.data);
        std::vector<std::string> args = split(trimed(str), " ");
        int argv = args.size();
        if (argv > 0) {
            if (exec_cmd(args, argv, inode_manager, spb) == -1)
                break;
        }

        send(cwd.c_str());

    }
    if (msgctl(msq_id_recv, IPC_RMID, 0) == -1){
        // 销毁消息队列
        std::cerr<<"Failed to destroy message queue!"<<std::endl;
        exit(-1);
    }
    exit(0);
}

void msq_service::login(std::vector<USER> users){
    /*
     * 用于和客户shell完成登录交互
     */
    int trials = 3;
    long int msg_to_receive = 0;
    std::string username;
    size_t passwd;
    while(trials--){
        send("Username:");

        recv();
        std::cout<<msg.data<<std::endl;
        username = trimed(msg.data);


        send("Password:");

        recv();
        std::cout<<msg.data<<std::endl;
        passwd = std::strtoull(std::string(msg.data).c_str() ,nullptr,10);
        for(auto _user:users){
            std::cout<<"ok"<<std::flush;
            if(_user.name==username && _user.passwd==passwd){
                user = _user;
                send("login success");
                recv();
                send((char*)&user);
                recv();
                send(cwd.c_str());
                recv();
                logged=true;
                return;
            }
        }
        send("login failed!");
    }
}

void msq_service::send(const char *buffer){
    /*
     * 向发送队列放入消息
     */
    msg.mtype = IPC_SERVER2CLI;
    strcpy(msg.data, buffer);
    if(msgsnd(msq_id_send, (void *)&msg, IPC_BUFFER_SIZE, 0)==-1){
        std::cerr<<"Failed to send!"<<std::endl;
        return;
    }
}

void msq_service::recv(){
    /*
     * 从接收队列拿出消息
     */
    long int msg_to_receive = 0;
    if(msgrcv(msq_id_recv, (void *)&msg, BUFSIZ, msg_to_receive, 0) == -1)
    {
        std::cerr<<"Failed to receive message!"<<std::endl;
        return;
    }
}

msq_service::~msq_service() {
    /*
     * 析构函数：
     *    销毁消息队列
     */
    msgctl(msq_id_recv, IPC_RMID, nullptr);
}
