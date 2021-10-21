//
// Created by 于承业 on 2021/10/20.
//
#include "util.h"

std::string& trim(std::string &str)
{
    /*
     * 去除str前后的空白字符，inplace操作，会修改传入的参数
     */
    if (str.empty()) return str;

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::string trimed(std::string str)
{
    /*
     * 去除str前后的空白字符，非inplace操作
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
    assert(mode/10==0);
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

std::string hex2bin(uint32 n)
{
    /*
     * 八进制数转二进制字符串
     */
    std::string ret="";
    while(n)
    {
        ushort t = n%10;
        n = n/10;
        ret += _hex2bin(t);
    }
    reverse(ret.begin(), ret.end());
    return ret;
}

std::string _hex2bin(ushort n){
    /*
     * 将一位八进制数转二进制字符串
     */
    if(n==0) return "000";
    if(n==1) return "001";
    if(n==2) return "010";
    if(n==3) return "011";
    if(n==4) return "100";
    if(n==5) return "101";
    if(n==6) return "110";
    if(n==7) return "111";
    return "000";
}

std::string mode2str(const ushort mode){
    /*
     * mode转读写执行权限的字符串
     * 例如755转为"rwxr-xr-x"
     */
    std::string mode_bin = hex2bin(mode);
    std::string ret = "---------";

    for(ushort i=0;i<9;++i){
        if(mode_bin[i]=='0') continue;
        if(i%3==0){
            ret[i] = 'r';
        }
        else if(i%3==1){
            ret[i] = 'w';
        }
        else{
            ret[i] = 'x';
        }
    }

    return ret;
}

ushort str2gid(const std::string& str){
    /*
     * 字符串转group id
     */
    if(str=="LOCAL") return LOCAL;
    if(str=="SYSTEM") return SYSTEM;
    if(str=="ROOT") return ROOT;
    if(str=="DEFAULT") return DEFAULT;
    return DEFAULT;
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
    return ret;
}


std::pair<ushort, bool>
path2inode_id(const std::string &path, inode_mgmt *inode_manager, ushort _uid, ushort _gid, ushort back_n) {
    /*
     * 路径转i节点号，注意路径需要是全路径
     * 如果back_n>0，则返回的是倒数第back_n级目录对应的i节点号
     * /dir1/dir2/
     */
    std::pair<ushort, bool> ret(0, false);
    std::vector<std::string> split_path = split(path, "/");

    if(split_path.empty() or (split_path.size()-back_n)<=0){
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

    for(ushort idx=0; idx<split_path.size()-back_n; ++idx){
        // 循环获取文件目录及i节点
        std::string name = split_path[idx];
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
                if(idx==split_path.size()-1-back_n){
                    ret.first = inode_id;
                    ret.second = true;
                    return ret;
                }
                res = inode_manager->read_dir_entry(inode_id, _uid, _gid);
                if(!res.second){
                    std::cerr<<"Failed to read dir entry!\n";
                    return ret;
                }
                curr_dir_entry = res.first;     // 当前目录项切换到子目录的目录项
                break;
            }
        }
    }

    // 说明filepath不存在
    std::cerr<<"[ERROR]No such file or directory: "<<path<<std::endl;

    ret.second = false;
    return ret;
}

std::pair<std::string, bool> concat_path(std::string a, std::string b){
    /*
     * 路径拼接：
     *   只审查路径语法上的合法性，而不考虑路径是否存在
     *   若合法，返回<path, true>
     *   否则，返回<"", false>
     */
    std::pair<std::string, bool> ret("", false);
    trim(a); trim(b);

    if(b[0]=='/'){
        // b在a后，因为b不能是绝对路径
        std::cerr<<"illegal path!\n";
        return ret;
    }

    // 格式化字符串
    a = format_path(a);
    b = format_path(b);

    ret.first = a+b;
    ret.second = true;
    return ret;
}

std::string format_path(std::string _path){
    /*
     * 审查和格式化路径字符串
     */
    _path = trimed(_path);
    if(_path=="/") return _path;

    std::vector<std::string> split_path = split(_path, "/");
    std::string prefix="";
    std::string ret="";

    if(_path[0]=='/'){
        prefix = "/";
    }

    ret += prefix;
    for(auto name:split_path){
        ret += (name + "/");
    }

    return ret;
}