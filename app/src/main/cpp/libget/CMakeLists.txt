# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.22.1)

file(GLOB SOURCES get.c get.h)

# 最终生成 libget.a 静态库
add_library(get STATIC ${SOURCES})
