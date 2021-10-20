//
// Created by 于承业 on 2021/10/20.
//
#include "util.h"

std::string& trim(std::string &str)
{
    /*
     * 去除str前后的空白字符
     */
    if (str.empty()) return str;

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    /*
     * 分割字符串
     */
    std::vector<std::string> ret;
    if(str == "") return ret;

    char *_str = new char[str.length() + 1];
    strcpy(_str, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(_str, d);
    while(p){
        std::string s = p; //分割得到的字符串转换为string类型
        ret.push_back(s); //存入结果数组
        p = strtok(nullptr, d);
    }

    return ret;
}

std::string mode_num2bin(const ushort mode){
    /*
     * 数字转二进制字符串
     */
    assert(mode%10==0);
    switch (mode) {
        case 0:
            return "000";
        case 1:
            return "001";
        case 3:
            return "011";
        case 4:
            return "100";
        case 5:
            return "101";
        case 6:
            return "110";
        case 7:
            return "111";
        default:
            return "000";
    }
}

std::vector<std::string> mode2str(const ushort mode){
    /*
     * mode转读写执行权限的字符串
     */
    ushort everyone = mode%10;
    ushort group = (mode/10)%10;
    ushort self = (mode/100);

    std::vector<std::string> ret;
    ret.push_back(mode_num2bin(self));
    ret.push_back(mode_num2bin(group));
    ret.push_back(mode_num2bin(everyone));

    return ret;
}

std::pair<std::vector<std::string>, bool> get_child_file(dir_entry entry, inode_mgmt* inode_manager){
    /*
     * 给定目录项，返回目录项对应的子文件名
     */
    std::vector<std::string> file_list;
    file_list.emplace_back(".");
    file_list.emplace_back("..");
    for(ushort i=2; i<entry.fileNum+2; ++i){
        ushort inode_id = entry.inode_arr[i];
        file_list.emplace_back(inode_manager->in_mem_inodes[inode_id].fileName);
    }
    auto ret = std::make_pair(file_list, true);
}


std::pair<ushort, bool> path2inode_id(char *path, inode_mgmt* inode_manager, ushort _uid, ushort _gid){
    /*
     * 路径转i节点号，注意路径需要是全路径
     * /dir1/dir2/
     */
    std::pair<ushort, bool> ret(0, false);
    std::string path_s = path;
    std::vector<std::string> split_path = split(path_s, "/");

    if(split_path.empty()){
        // 说明是根目录
        ret.first = 0;
        ret.second = true;
        return ret;
    }

    dir_entry curr_dir_entry;
    auto res = inode_manager->read_dir_entry(0, _uid, _gid);    // 读取根目录的目录项
    if(!res.second){
        std::cerr<<"Failed to read dir entry!\n";
        return ret;
    }
    curr_dir_entry = res.first;

    for(auto name: split_path){
        // 循环获取文件目录及i节点
        auto res2 = get_child_file(curr_dir_entry, inode_manager);
        if(!res2.second){
            std::cerr<<"Failed to get child file list!\n";
            return ret;
        }
        std::vector<std::string> &filelist = res2.first;

        for(ushort i=2; i<filelist.size()+2; ++i){
            // 查找目录名对应的i节点号
            if(filelist[i] == name){
                ushort inode_id = curr_dir_entry.inode_arr[i];
                res = inode_manager->read_dir_entry(inode_id, _uid, _gid);
                if(!res.second){
                    std::cerr<<"Failed to read dir entry!\n";
                    return ret;
                }
                curr_dir_entry = res.first;
                if(name==split_path.back()){
                    ret.first = inode_id;
                    ret.second = true;
                    return ret;
                }
                break;
            }
        }
    }

    return ret;
}