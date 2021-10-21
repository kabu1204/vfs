//
// Created by yuchengye on 2021/10/22.
//
#include "init.h"

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

    std::hash<std::string> hash_fn;
    char default_user_info[256];
    std::string fmt("# DO NOT EDIT\n# User Attributes\ngroup SYSTEM\ngroup\tLOCAL\ngroup\tROOT\ngroup\tDEFAULT\nuser\troot:%lu:1:ROOT\nuser\tycy:%lu:2:ROOT\n");
    size_t real_size = std::snprintf(default_user_info, sizeof(default_user_info), fmt.c_str(), hash_fn("passwd_root"), hash_fn("passwd_ycy"));

    vfstream f(inode_manager, "/.user", 'w', SYSTEM, SYSTEM);
    f.write(default_user_info, real_size);
    f.close();

    uint32 size = inode_manager->getsizeof("/.user", SYSTEM, SYSTEM);
    char *buffer = new char[size];
    f.open("/.user", 'r');
    uint32 n = f.read(buffer, size);
    std::cout<<buffer;
    f.close();

    return 1;
}