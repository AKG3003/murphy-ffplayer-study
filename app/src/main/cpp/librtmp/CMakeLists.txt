cmake_minimum_required(VERSION 3.22.1)

file(GLOB_RECURSE rtmp_src *.c)

# 如果不修改源码的情况下，解决报错，添加宏 rtmp.c
# 这里注意，因为rtmp是全c编译，所以不需要用cxx方式，CMAKE_C_FLAGS 而不是 CMAKE_CXX_FLAGS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library(rtmp STATIC ${rtmp_src})