cmake_minimum_required(VERSION 3.20)
project(vfs)

set(CMAKE_CXX_STANDARD 11)

add_executable(vfs main.cpp superblock.cpp superblock.h inode.cpp inode.h types.h rights.cpp rights.h dir.cpp dir.h)
