# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.22.1)

file(GLOB SOURCES count.cpp count.h)

# 最终生成libcount.so静态库文件
add_library(count SHARED ${SOURCES})
