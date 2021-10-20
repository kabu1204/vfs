#ifndef VFS_UTIL_H
#define VFS_UTIL_H
#include "string"
#include "vector"

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
        p = strtok(NULL, d);
    }

    return ret;
}

#endif //VFS_UTIL_H
