cmake_minimum_required(VERSION 3.22.1)

project("ndk22_compile_study")

set(FFMPEG ${CMAKE_SOURCE_DIR}/ffmpeg)
message(FFMPEG: ${FFMPEG})

include_directories(${FFMPEG})

file(GLOB SOURCES *.cpp *.c *.h *.hpp)
message(SOURCES: ${SOURCES})
# 添加头文件目录

#message(CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR})

# 设置编译参数
if (DEFINED ANDROID_ABI)
    set(STUDY_ABI ${ANDROID_ABI})
else ()
    set(STUDY_ABI "arm64-v8a")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/../jniLibs/${STUDY_ABI}")

add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

#add_subdirectory(${CMAKE_SOURCE_DIR}/libcount)

#add_subdirectory(${CMAKE_SOURCE_DIR}/libget)

#find_library(android-lib android)
#message(android-lib: ${android-lib})
#find_library(z-lib z)
#message(z-lib: ${z-lib})
#find_library(OpenSLES-lib OpenSLES)
#message(OpenSLES-lib: ${OpenSLES-lib})
#find_library(rtmp-lib rtmp)
#message(rtmp-lib: ${rtmp-lib})

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        avutil
        avcodec
        avformat
        avfilter
        swresample
        swscale
        android
        log
        z
        Android
        OpenSLES
)