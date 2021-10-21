cmake_minimum_required(VERSION 3.20)
project(vfs)

set(CMAKE_CXX_STANDARD 11)

add_executable(vfs main.cpp superblock.cpp superblock.h inode.cpp inode.h types.h rights.cpp rights.h dir_entry.cpp dir_entry.h ioservice.cpp ioservice.h util.h users.h vfstream.cpp vfstream.h util.cpp init.h init.cpp)
