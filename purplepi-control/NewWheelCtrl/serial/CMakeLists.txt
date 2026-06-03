cmake_minimum_required(VERSION 3.10)
project(serial)

# 设置源文件为本文件夹下的所有.c .cpp文件以及lcmtype文件夹下的所有.c .cpp文件
aux_source_directory(. DIR_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/lcmtype DIR_SRCS)

add_executable(serial ${DIR_SRCS})

# 添加链接库
target_link_libraries(serial lcm pthread)
