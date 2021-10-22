#include "main.h"

void simdisk(inode_mgmt *inode_manager, superblock_c *spb);

//int exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb);

std::pair<USER, bool> login(const std::vector<USER> &users);

void prompt();

std::vector<USER> load_User(inode_mgmt* inode_manager);

std::string cwd = "/";      // 当前工作目录
ushort cwd_inode_id = 0;    // 当前工作目录对应的i节点号
// 当前登录用户，在登录之前的初始化或加载过程中，以SYSTEM身份进行
// 登录完成后，切换到登录用户
USER user("system", 0, SYSTEM, SYSTEM);

int main(int argc, char **argv) {
    if(argc>=2){
        // 使用./vfs client命令开启客户端
        // 顺序是先开服务端，再开客户端
        // 运行完成后需要执行ipcrm清除共享内存
        if(strcmp(argv[1], "client")==0){
            shell sh("./ipc_msg", 123);
        }
    }
    ioservice *io_context = nullptr;        // 读写服务
    inode_mgmt *inode_manager = nullptr;    // i节点管理器
    superblock_c *spb = nullptr;            // 超级块管理器
    Format(inode_manager, io_context, spb);   // 格式化磁盘
//    Load(spb, io_context, inode_manager);   // 加载磁盘数据


    std::vector<USER> users = load_User(inode_manager); // 加载用户信息

    msq_service msq("./ipc_msg", 123, users, inode_manager, spb, cwd);

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
            if(exec_cmd(args, argv, inode_manager, spb) == -1)
                break;
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

int exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb) {
    std::string cmd = args[0];
    if(cmd == "info"){
        spb->print();
        uint32 used_block_num = spb->s_block.max_block_num - spb->s_block.free_block_num;   // 已用磁盘块数量
        double used_percent = 100*double(used_block_num)/spb->s_block.max_block_num;            // 已用百分比
        char root_dir_name[MAX_FILENAME_LEN];       // 根目录名
        strcpy(root_dir_name, inode_manager->in_mem_inodes[0].fileName);    // 0号i节点对应根目录
        std::cout<<std::endl<<std::flush<<"Filesystem\tBlocks\tUsed\tFree\tUsed%\tMounted on"<<std::endl;
        std::printf("vfs.disk\t%u\t%u\t%u\t%.2f%%\t%s\n",spb->s_block.max_block_num,used_block_num,spb->s_block.free_block_num,used_percent,root_dir_name);
    }
    else if(cmd == "cd"){
        /*
         * 改变当前工作目录
         */
        std::string target_dir;
        if(argv==1){
            return 0;
        }
        if(trimed(args[1])[0]=='/'){
            target_dir = trimed(args[1]);
        }
        else{
            auto res = concat_path(cwd, args[1]);
            if(!res.second) return 0; // 路径非法
            target_dir = res.first;
        }
        auto res2 = inode_manager->get_inode_id(target_dir, user.uid, user.gid);
        if(!res2.second){
            // 路径不存在
            std::cerr<<"No such directory!\n";
            return 0;
        }
        cwd_inode_id = res2.first;  // 设置当前工作目录的i节点号
        cwd = inode_manager->get_full_path_of(cwd_inode_id, user.uid, user.gid).first;    // 设置当前工作目录
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
                if(!res.second) return 0; // 路径非法
                target_dir = res.first;
            }
            auto res2 = inode_manager->get_inode_id(target_dir, user.uid, user.gid);
            if(!res2.second){    // 路径不存在
                std::cerr<<"No such directory!\n";
                return 0;
            }
            target_dir_inode_id = res2.first;
            if(!inode_manager->isDir(target_dir_inode_id)){
                std::cerr<<"The path is not a directory!";
                return 0;
            }
        }
        auto res3 = inode_manager->get_dir_entry(target_dir_inode_id, user.uid, user.gid);
        if(!res3.second) return 0;    // 获取目录项失败
        dir_entry& entry = res3.first;
        ushort curr_inode_id;
        std::printf("Protection\tAddr\tSize\tName\n");
        for(ushort i=0;i<entry.fileNum+2;++i){
            curr_inode_id = entry.inode_arr[i];
            std::string protection_info = inode_manager->get_protection_code(curr_inode_id).first;
            uint32 addr = inode_manager->get_addr_of(curr_inode_id).first;
            size_t size = inode_manager->getsizeof(curr_inode_id);
            std::string name = inode_manager->get_name_of(curr_inode_id).first;
            if(i==0) name = ".";
            if(i==1) name = "..";
            std::printf("%s\t%u\t%d\t\t%s\n", protection_info.c_str(), addr, size, name.c_str());
        }
    }
    else if(cmd == "md" or cmd == "mkdir"){
        /*
         * 创建目录
         */
        if(argv==1){
            std::cout<<"usage: md|mkdir directory\n";
            return 0;
        }
        std::string target_dir;
        std::string formatted_path = format_path(args[1]);
        if(formatted_path[0]=='/'){
            target_dir = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second) {
                std::cerr<<"No such file or directory!\n";
                return 0;
            }
            target_dir = res.first;
        }
        if(!inode_manager->mkdir(target_dir, user.uid, user.gid).second){
            return 0;
        }
    }
    else if(cmd == "rd"){
        /*
         * 删除目录
         */
        if(argv<2){
            std::printf("usage: rd directory\n");
            return 0;
        }
        std::string formatted_path = format_path(args[1]);
        std::string target_dir;
        if(formatted_path[0]=='/'){
            target_dir = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second) return 0; // 路径非法
            target_dir = res.first;
        }
        auto res = inode_manager->get_full_path_of(target_dir, user.uid, user.gid);
        if(!res.second){
            std::cerr<<"No such file or directory!\n";
            return 0;
        }
        if(res.first.size()>cwd.size() and res.first.find(cwd)!=res.first.npos){
            // 代表target_dir是cwd的子路径
            ushort inode_id = inode_manager->get_inode_id(res.first, user.uid, user.gid).first;
            if(inode_manager->isDir(inode_id) && inode_manager->open_dir_entry(inode_id,user.uid, user.gid).first.size()>2){
                // 非空目录，询问用户
                std::cout<<"Directory is not empty, are you sure to remove it recursively? [y/n (default:n)]:";
                std::string user_in;
                std::getline(std::cin, user_in);
                if((trimed(user_in))!="y") return 0;  // 用户拒绝操作
            }
            inode_manager->rmdir(res.first, user.uid, user.gid);
        }
        else{
            std::cerr<<"You are removing a directory where your current working directory is located!"<<std::endl;
            return 0;
        }
    }
    else if(cmd == "newfile"){
        /*
         * 建立新文件
         */
        if(argv==1){
            std::cout<<"usage: newfile filepath\n";
            return 0;
        }
        std::string formatted_path = format_path(args[1]);
        std::string target_path;
        if(formatted_path[0]=='/'){
            target_path = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second) return 0; // 路径非法
            target_path = res.first;
        }
        inode_manager->mkfile(target_path, user.uid, user.gid);
    }
    else if(cmd == "cat"){
        /*
         * 输出文件内容
         */
        if(argv==1){
            std::cout<<"usage: cat filepath\n";
            return 0;
        }
        std::string formatted_path = format_path(args[1]);
        std::string target_path;
        if(formatted_path[0]=='/'){
            target_path = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second){
                std::cerr<<"No such file!\n"; // 路径不存在
                return 0;
            }
            target_path = res.first;
        }
        vfstream f(inode_manager, target_path, 'r', user.uid, user.gid);   // 创建虚拟文件流
        size_t buffer_size = inode_manager->getsizeof(target_path, user.uid, user.gid);
        char *buffer = new char[buffer_size];
        f.read(buffer, buffer_size);
        f.close();
        std::cout<<buffer;
    }
    else if(cmd == "copy"){
        /*
         * 拷贝文件
         */
        if(argv<3){
            std::cout<<"usage:\n\tcopy src dst --- copy from src to dst\n\tcopy <host>src dst --- copy from host's src path to virtual disk's dst";
            return 0;
        }
        std::string src = args[1];
        std::string dst = args[2];
        char buffer[BLOCK_SIZE];
        size_t n;

        if(dst.substr(0,6)!="<host>") {
            auto res = inode_manager->get_inode_id(dst, user.uid, user.gid);
            if (!res.second){
                // 路径不存在，尝试创建
                std::vector<std::string> args_t;     // 参数
                args_t.push_back("newfile");
                args_t.push_back(dst);
                exec_cmd(args_t, 2, inode_manager, spb);
            }
            else {
                // 路径存在
                if (inode_manager->isDir(res.first)) {
                    // 如果dst是个目录，则需要首先创建同名文件
                    // 直接用exec_cmd已经实现的newfile命令
                    std::string filename = split(src, "/").back();
                    std::vector<std::string> args_t;     // 参数
                    args_t.push_back("newfile");
                    args_t.push_back(concat_path(dst, filename).first);
                    exec_cmd(args_t, 2, inode_manager, spb);
                    dst = concat_path(dst, filename).first;
                }
            }
        }

        if(src.substr(0,6)=="<host>" and dst.substr(0,6)=="<host>"){
            std::ifstream f_read(src.substr(6, src.size()-6).c_str(), std::ios::in|std::ios::binary);
            std::ofstream f_write(dst.substr(6, dst.size()-6), std::ios::out|std::ios::binary);
            while(!f_read.eof()){
                f_read.read(buffer, BLOCK_SIZE);
                n = f_read.gcount();
                f_write.write(buffer, n);
            }
            f_read.close();
            f_write.close();
        }
        else if(src.substr(0,6)=="<host>" and dst.substr(0,6)!="<host>"){
            std::ifstream f_read(src.substr(6, src.size()-6).c_str(), std::ios::in|std::ios::binary);
            vfstream f_write(inode_manager, dst, 'w', user.uid, user.gid);
            while(!f_read.eof()){
                f_read.read(buffer, BLOCK_SIZE);
                n = f_read.gcount();
                f_write.write(buffer, n);
            }
            f_read.close();
            f_write.close();
        }
        else if(src.substr(0,6)!="<host>" and dst.substr(0,6)!="<host>"){
            vfstream f_read(inode_manager, src, 'r', user.uid, user.gid);
            vfstream f_write(inode_manager, dst, 'w', user.uid, user.gid);
            while(!f_read.eof()){
                n = f_read.read(buffer, BLOCK_SIZE);
                f_write.write(buffer, n);
            }
            f_read.close();
            f_write.close();
        }
        else{
            vfstream f_read(inode_manager, src, 'r', user.uid, user.gid);
            std::ofstream f_write(dst.substr(6, dst.size()-6).c_str(), std::ios::out|std::ios::binary);
            while(!f_read.eof()){
                n = f_read.read(buffer, BLOCK_SIZE);
                f_write.write(buffer, n);
            }
            f_read.close();
            f_write.close();
        }
    }
    else if(cmd == "del"){
        if(argv==1){
            std::cout<<"usage: del filepath\n";
            return 0;
        }
        std::string formatted_path = format_path(args[1]);
        std::string target_path;
        if(formatted_path[0]=='/'){
            target_path = formatted_path;
        }
        else{
            auto res = concat_path(cwd, formatted_path);
            if(!res.second){
                std::cerr<<"No such file!\n";
                return 0;
            }
            target_path = res.first;
        }
        // 错误提示由rmfile方法给出
        inode_manager->rmfile(target_path, user.uid, user.gid);
    }
    else if(cmd == "check"){
        uint32 inode_held_blocks = inode_manager->check_and_correct();
        spb->check_and_correct(inode_held_blocks);
        return 0;
    }
    else if(cmd == "EXIT"){
        return -1;
    }
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
            size_t passwd = std::strtoull(pattern[1].c_str(), nullptr, 10);
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