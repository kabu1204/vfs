#include "main.h"
#include "types.h"
#include "superblock.h"
#include "inode.h"
#include "util.h"
#include "ioservice.h"
#include "users.h"
#include <iostream>
#include <string>
#include <cstdio>


int Init(inode_mgmt *&inode_manager, ioservice *&io_context, superblock_c *&spb);

void init_inodeSpace(std::ofstream *disk, superblock_c *spb);

void init_Superblock(std::ofstream *disk, superblock_c* &spb);

int init_rootDir(superblock_c *spb, ioservice *io_context, inode_mgmt* &inode_manager);

int init_DefaultUsers(superblock_c *spb, ioservice *io_context, inode_mgmt  *inode_manager);

void simdisk(inode_mgmt *inode_manager, superblock_c *spb);

void exec_cmd(std::vector<std::string> args, int argv, inode_mgmt *inode_manager, superblock_c *spb);

int main() {
    auto res = split("/dir1/","/");
    std::cout<<res.size()<<"\n"<<res[0];
//    ioservice *io_context = nullptr;        // 读写服务
//    inode_mgmt *inode_manager = nullptr;    // i节点管理器
//    superblock_c *spb = nullptr;            // 超级块管理器
//    Init(inode_manager, io_context, spb);   // 初始化磁盘
//    return 0;
}

void simdisk(inode_mgmt *inode_manager, superblock_c *spb) {
    /*
     * 只用于作业的设计1：
     *    串行的模拟shell，根据用户输入执行命令，管理磁盘
     */
    ushort cwd_inode_id = 0;    // 当前工作目录对应的i节点号
    std::string cwd;            // 当前工作目录路径
    std::cout<<"> ";
    std::string str;
    while(std::getline(std::cin, str))
    {
        str = trim(str);
        std::vector<std::string> args = split(str, " ");
        int argv = args.size();
        if(argv > 0){

        }
    }
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

    }
}

int Init(inode_mgmt *&inode_manager, ioservice *&io_context, superblock_c *&spb) {
    /*
     * 功能：初始化磁盘
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

    init_Superblock(&disk, spb);    // 初始化超级块区
    init_inodeSpace(&disk, spb);       // 初始化i节点区

    disk.close();
    std::cout << "Initialize superblock and inodes successfully!\n";

    io_context = new ioservice("vfs.disk", spb->s_block.superblock_start_addr, spb->s_block.inode_start_addr, spb->s_block.root_dir_start_addr, sizeof(inode));

    if(init_rootDir(spb, io_context, inode_manager) != 1)   // 初始化根目录区
    {
        std::cerr<<"Failed to initialize root dir_entry!\n";
        return 0;
    }
    std::cout<<"Initialize root dir_entry successfully!\n";
    spb->print();

    return 1;
}

void init_Superblock(std::ofstream *disk, superblock_c* &spb) {
    /*
     * 功能：初始化超级块
     */
    char fill_char = '\0';  // 空白填充字符
    spb = new superblock_c(0, MAX_SPACE, BLOCK_SIZE, MAX_INODE_NUM);  // 建立新的superblock

    uint32 superblock_size_n = sizeof(spb->s_block);  // superblock实际所占字节数
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

void init_inodeSpace(std::ofstream *disk, superblock_c *spb) {
    /*
     * 初始化inode区
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

int init_rootDir(superblock_c *spb, ioservice *io_context, inode_mgmt* &inode_manager) {
    inode_manager = new inode_mgmt(spb, io_context);
    auto ret = inode_manager->mkdir("/", 0, SYSTEM, SYSTEM);
    if(!ret.second){
        std::cerr<<"Failed to create root dir_entry!\n";
        return 0;
    }
    return 1;
}

int init_DefaultUsers(superblock_c *spb, ioservice *io_context, inode_mgmt  *inode_manager){

}