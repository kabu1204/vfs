#include "main.h"
#include "types.h"
#include "superblock.h"
#include "inode.h"
#include "util.h"
#include "ioservice.h"
#include "users.h"
#include "vfstream.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <functional>

int Load(superblock_c *&spb, ioservice *&io_context, inode_mgmt *&inode_manager);

int Format(inode_mgmt *&inode_manager, ioservice *&io_context, superblock_c *&spb);

void format_inodeSpace(std::ofstream *disk, superblock_c *spb);

void format_Superblock(std::ofstream *disk, superblock_c* &spb);

int format_rootDir(superblock_c *spb, ioservice *io_context, inode_mgmt* &inode_manager);

int init_DefaultUsers(superblock_c *spb, ioservice *io_context, inode_mgmt  *inode_manager);

void simdisk(inode_mgmt *inode_manager, superblock_c *spb);

void exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb);

std::vector<USER> load_User(inode_mgmt* inode_manager);

std::pair<USER, bool> login(const std::vector<USER> &users);

void prompt();

std::string cwd = "/";      // 当前工作目录
ushort cwd_inode_id = 0;    // 当前工作目录对应的i节点号
// 当前登录用户，在登录之前的初始化或加载过程中，以SYSTEM身份进行
// 登录完成后，切换到登录用户
USER user("system", 0, SYSTEM, SYSTEM);

int main() {
    ioservice *io_context = nullptr;        // 读写服务
    inode_mgmt *inode_manager = nullptr;    // i节点管理器
    superblock_c *spb = nullptr;            // 超级块管理器
//    Format(inode_manager, io_context, spb);   // 格式化磁盘
    Load(spb, io_context, inode_manager);   // 加载磁盘数据

    std::vector<USER> users = load_User(inode_manager);
    std::pair<USER, bool> res = login(users);
    if(res.second){
        // 登录成功
        simdisk(inode_manager, spb);
    }

    delete inode_manager;
    delete spb;
    delete io_context;
    return 0;
}

void simdisk(inode_mgmt *inode_manager, superblock_c *spb) {
    /*
     * 只用于作业的设计1：
     *    串行的模拟shell，根据用户输入执行命令，管理磁盘
     */
    std::string str;
    prompt();
    while(std::getline(std::cin, str))
    {
        std::vector<std::string> args = split(trimed(str), " ");
        int argv = args.size();
        if(argv > 0){
            exec_cmd(args, argv, inode_manager, spb);
        }
        prompt();
    }
}

void prompt() {
    /*
     * 打印终端的提示信息
     */
    char prefix = (user.gid==ROOT) ? '#' : '$';     // #代表具有ROOT权限的用户   $代表普通用户
    //格式为 "prefix username in cwd"， 如 # root in /home/Document
    std::printf("\033[31m%c \033[1m%s\033[0m \033[36min\033[0m \033[1m\033[4m\033[37m%s\033[0m ", prefix, user.name, cwd.c_str());
}

void exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb) {
    std::string cmd = args[0];
    if(cmd == "info"){
        spb->print();
        uint32 used_block_num = spb->s_block.max_block_num - spb->s_block.free_block_num;   // 已用磁盘块数量
        double used_percent = 100*double(used_block_num)/spb->s_block.max_block_num;            // 已用百分比
        char root_dir_name[MAX_FILENAME_LEN];       // 根目录名
        strcpy(root_dir_name, inode_manager->in_mem_inodes[0].fileName);    // 0号i节点对应根目录
        std::cout<<"\nFilesystem\tBlocks\tUsed\tFree\tUsed%\tMounted on\n";
        std::printf("vfs.disk\t%u\t%u\t%u\t%.2f%%\t%s",spb->s_block.max_block_num,used_block_num,spb->s_block.free_block_num,used_percent,root_dir_name);
    }
    else if(cmd == "cd"){
        /*
         * 改变当前工作目录
         */
        std::string target_dir;
        if(argv==1){
            return;
        }
        if(trimed(args[1])[0]=='/'){
            target_dir = trimed(args[1]);
        }
        else{
            auto res = concat_path(cwd, args[1]);
            if(!res.second) return; // 路径非法
            target_dir = res.first;
        }
        auto res2 = path2inode_id(target_dir, inode_manager, user.uid, user.gid, 0);
        if(!res2.second) return;    // 路径不存在或者没有权限
        cwd_inode_id = res2.first;  // 设置当前工作目录的i节点号
        cwd = inode_manager->get_full_path_of(cwd_inode_id, user.uid, user.gid).first;    // 设置当前工作目录
    }
    else if(cmd == "info"){
        spb->print();
    }
    else if(cmd == "dir"){
        /*
         * 打印目录信息
         */
        ushort target_dir_inode_id;
        if(argv==1) target_dir_inode_id = cwd_inode_id;
        else{
            std::string target_dir;
            if(trimed(args[1])[0]=='/'){
                target_dir = trimed(args[1]);
            } else {
                auto res = concat_path(cwd, args[1]);
                if(!res.second) return; // 路径非法
                target_dir = res.first;
            }
            auto res2 = path2inode_id(target_dir, inode_manager, user.uid, user.gid, 0);
            if(!res2.second) return;    // 路径不存在或者没有权限
            target_dir_inode_id = res2.first;
            if(!inode_manager->isDir(target_dir_inode_id)){
                std::cerr<<"The path is not a directory!";
                return;
            }
        }
        auto res3 = inode_manager->get_dir_entry(target_dir_inode_id, user.uid, user.gid);
        if(!res3.second) return;    // 获取目录项失败
        dir_entry& entry = res3.first;
        ushort curr_inode_id;
        std::printf("Protection\tAddr\tSize\tName");
        for(ushort i=0;i<entry.fileNum+2;++i){
            curr_inode_id = entry.inode_arr[i];
            std::string protection_info = inode_manager->get_protection_code(curr_inode_id).first;
            uint32 addr = inode_manager->get_addr_of(curr_inode_id).first;
            size_t size = inode_manager->getsizeof(curr_inode_id);
            std::string name = inode_manager->get_name_of(curr_inode_id).first;
            if(i==0) name = ".";
            if(i==1) name = "..";
            std::printf("%s\t%u\t%d\t%s\n", protection_info.c_str(), addr, size, name.c_str());
        }
    }
    else if(cmd == "md" or cmd == "mkdir"){
        /*
         * 创建目录
         */
        if(argv==0){
            std::cout<<"usage: md|mkdir directory\n";
            return;
        }
        std::string target_dir;
        std::string formatted_path = format_path(args[1]);
        if(formatted_path[0]=='/'){
            target_dir = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second) return; // 路径非法
            target_dir = res.first;
        }
        if(!inode_manager->mkdir(target_dir, user.uid, user.gid).second){
            return;
        }
    }
    else if(cmd == "rd"){
        /*
         * 删除目录
         */
        if(argv<2){
            std::printf("usage: rd directory\n");
            return;
        }
        std::string formatted_path = format_path(args[1]);
        std::string target_dir;
        if(formatted_path[0]=='/'){
            target_dir = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second) return; // 路径非法
            target_dir = res.first;
        }
    }
}


int Load(superblock_c *&spb, ioservice *&io_context, inode_mgmt *&inode_manager) {
    /*
     * 磁盘已经保存了内容，直接从磁盘上加载超级块、i节点等信息
     */
    spb = new superblock_c(0, MAX_SPACE, BLOCK_SIZE, MAX_INODE_NUM);  // 建立新的superblock
    uint32 superblock_size_n = sizeof(spb->s_block);  // superblock实际大小
    uint32 superblock_block_n;    // superblock所占block数量

    std::ifstream disk("vfs.disk", std::ios::in | std::ios::binary);
    spb->load_super_block(&disk);   // 读取superblock
    disk.close();

    io_context = new ioservice("vfs.disk", spb->s_block.superblock_start_addr, spb->s_block.inode_start_addr, spb->s_block.root_dir_start_addr, sizeof(inode));
    spb->set_io_context(io_context);

    inode_manager = new inode_mgmt(spb, io_context);


    vfstream f(inode_manager, "/.user", 'r', SYSTEM, SYSTEM);;
    uint32 size = inode_manager->getsizeof("/.user", SYSTEM, SYSTEM);
    char *buffer = new char[size];
    uint32 n = f.read(buffer, size);
    std::cout<<buffer;

    spb->print();

    return 1;
}

std::vector<USER> load_User(inode_mgmt* inode_manager){
    /*
     * 读取系统文件/.user，加载用户信息
     */
    uint32 size = inode_manager->getsizeof("/.user", SYSTEM, SYSTEM);
    char *user_info = new char[size];

    vfstream f(inode_manager, "/.user", 'r', SYSTEM, SYSTEM);
    f.read(user_info, size);

    std::vector<USER> users;
    std::vector<std::string> lines = split(std::string(user_info), "\n");   // 按行分割
    for(auto line:lines){
        if(trimed(line)[0]=='#') continue;
        std::vector<std::string> record = split(std::string(line), "\t");   // 解析一条有效记录
        if(record[0]=="group"){
            // TODO
            continue;
        }
        if(record[0]=="user"){
            std::vector<std::string> pattern = split(std::string(record[1]), ":");  // 解析模式： username:hash_fn(passwd):uid:group
            std::string username = pattern[0];
            size_t passwd = std::strtoll(pattern[1].c_str(), nullptr, 10);
            std::string uid = pattern[2];
            std::string group = pattern[3];
            users.emplace_back(pattern[0], passwd, std::strtol(uid.c_str(), nullptr, 10),str2gid(group));
        }
    }
    return users;
}

std::pair<USER, bool> login(const std::vector<USER> &users) {
    /*
     * 登录
     */
    std::hash<std::string> hash_fn;
    ushort trials = 3;  // 最多尝试3次
    std::string username;
    std::string passwd;
    while(trials--){
        std::cout<<"Username:";
        std::getline(std::cin, username);
        for(auto user:users){
            if(user.name==trimed(username)){
                std::cout<<"Password:";
                std::getline(std::cin, passwd);
                if(hash_fn(passwd)==user.passwd){
                    // 登录成功
                    return std::make_pair(user, true);
                }
                break;
            }
        }
        std::cout<<"Incorrect username or password!\n";
    }
    std::cout<<"You wasted your 3 chances, please retry!\n";
    return std::make_pair(USER("",0,0,0), false);
}

int Format(inode_mgmt *&inode_manager, ioservice *&io_context, superblock_c *&spb) {
    /*
     * 功能：格式化磁盘
     */
    char fill_char = '\0';  // 空白填充字符

    // 打印提示信息
    std::cout << "Creating file system and formatting the disk space ...\n";
    std::printf("Total disk space: %.2fMB\n", double(MAX_SPACE) / 1024 / 1024);

    // 建立虚拟磁盘
    std::ofstream disk("vfs.disk", std::ios::out | std::ios::binary);
    for (uint32 i = 0; i < MAX_SPACE; ++i)
        disk.write(&fill_char, sizeof(fill_char));  // 写入填充字符

    std::cout << "formatting finished\n";

    format_Superblock(&disk, spb);    // 初始化超级块区
    format_inodeSpace(&disk, spb);       // 初始化i节点区

    disk.close();
    std::cout << "Initialize superblock and inodes successfully!\n";

    io_context = new ioservice("vfs.disk", spb->s_block.superblock_start_addr, spb->s_block.inode_start_addr, spb->s_block.root_dir_start_addr, sizeof(inode));
    spb->set_io_context(io_context);    // 设置superblock_c的io服务

    if(format_rootDir(spb, io_context, inode_manager) != 1)   // 初始化根目录区
    {
        std::cerr<<"Failed to initialize root dir_entry!\n";
        return 0;
    }
    std::cout<<"Initialize root dir_entry successfully!\n";

    if(init_DefaultUsers(spb, io_context, inode_manager)!=1){
        std:std::cerr<<"Failed to initialize default users!\n";
        return 0;
    }

    spb->print();   // 打印系统信息
    return 1;
}

void format_Superblock(std::ofstream *disk, superblock_c* &spb) {
    /*
     * 功能：格式化超级块区域
     */
    char fill_char = '\0';  // 空白填充字符
    spb = new superblock_c(0, MAX_SPACE, BLOCK_SIZE, MAX_INODE_NUM);  // 建立新的superblock_c

    uint32 superblock_size_n = sizeof(spb->s_block);  // superblock实际大小
    uint32 superblock_block_n;    // superblock所占block数量
    superblock_block_n = (superblock_size_n % BLOCK_SIZE == 0) ? superblock_size_n / BLOCK_SIZE :
                         superblock_size_n / BLOCK_SIZE + 1;

    // 以下是手动为superblock分配空间
    spb->s_block.free_block_num -= superblock_block_n;
    spb->s_block.free_space -= superblock_block_n * BLOCK_SIZE;    // 剩余空间需要减去superblock所占空间。
    spb->s_block.inode_start_addr = spb->s_block.superblock_start_addr +
                                    superblock_block_n * spb->s_block.block_size;   // i节点起始地址是superblock结束地址+1
    spb->s_block.superblock_size = superblock_size_n;    // superblock占用字节数
    spb->s_block.last_p = superblock_block_n;
    // superblock占用磁盘块置为已分配状态
    for (int i = 0; i < superblock_block_n; ++i)
        spb->s_block.bitmap.set(i);


    disk->write((char *) &(spb->s_block), superblock_size_n);   // 写入superblock
    for (int i = 0; i < (superblock_block_n * BLOCK_SIZE - superblock_size_n); ++i)
        disk->write(&fill_char, sizeof(fill_char));   //剩余空间填入空白字符

}

void format_inodeSpace(std::ofstream *disk, superblock_c *spb) {
    /*
     * 格式化inode区
     */
    inode t;
    uint32 inodes_size_n = MAX_INODE_NUM * sizeof(t);
    uint32 inodes_block_n = (inodes_size_n % BLOCK_SIZE == 0) ? inodes_size_n / BLOCK_SIZE : inodes_size_n / BLOCK_SIZE + 1;


    // 以下是手动为inodes区分配空间
    spb->s_block.free_block_num -= inodes_block_n;
    spb->s_block.free_space -= inodes_block_n * BLOCK_SIZE;    // 剩余空间需要减去inodes所占空间。
    spb->s_block.root_dir_start_addr =
            spb->s_block.inode_start_addr + inodes_block_n * BLOCK_SIZE;   // 根目录区起始地址是inodes结束地址+1
    // inodes占用磁盘块置为已分配状态
    for (uint32 i = spb->s_block.last_p; i < inodes_block_n; ++i)
        spb->s_block.bitmap.set(i);
    spb->s_block.last_p += inodes_block_n;

    disk->seekp(spb->s_block.inode_start_addr);
    for (ushort i = 0; i < MAX_INODE_NUM; ++i) {
        t.id = i;
        disk->write((char *) &t, sizeof(t));
    }

}

int format_rootDir(superblock_c *spb, ioservice *io_context, inode_mgmt* &inode_manager) {
    /*
     * 格式化根目录区
     */
    inode_manager = new inode_mgmt(spb, io_context);        // 初始化i节点管理器
    auto ret = inode_manager->mkdir("/", SYSTEM, SYSTEM);    // 根目录区属于系统创建的目录
    if(!ret.second){
        std::cerr<<"Failed to create root dir_entry!\n";
        return 0;
    }
    return 1;
}

int init_DefaultUsers(superblock_c *spb, ioservice *io_context, inode_mgmt  *inode_manager){
    /*
     * 初始化默认用户信息，存在根目录下的.user文件中
     */
    auto ret = inode_manager->mkfile("/.user", SYSTEM, SYSTEM);
    if(!ret.second){
        std::cerr<<"Failed to create \".user\" file!\n";
        return 0;
    }

    char default_user_info[128]="# DO NOT EDIT\n# User Attributes\ngroup SYSTEM\ngroup\tLOCAL\ngroup\tROOT\ngroup\tDEFAULT\nuser\troot:1:ROOT\nuser\tycy:2:ROOT\n";
    vfstream f(inode_manager, "/.user", 'w', SYSTEM, SYSTEM);
    f.write(default_user_info, sizeof(default_user_info));
    f.close();

    uint32 size = inode_manager->getsizeof("/.user", SYSTEM, SYSTEM);
    char *buffer = new char[size];
    f.open("/.user", 'r');
    uint32 n = f.read(buffer, size);
    std::cout<<buffer;
    f.close();

    return 1;
}