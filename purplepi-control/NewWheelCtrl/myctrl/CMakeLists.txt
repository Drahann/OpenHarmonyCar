cmake_minimum_required(VERSION 3.10)
project(myctrl)

# 添加头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/../udp2lcm
)

# 设置源文件为本文件夹下的所有.c .cpp文件以及lcmtype文件夹下的所有.c .cpp文件
aux_source_directory(. DIR_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/lcmtype DIR_SRCS)

# 生成可执行文件
add_executable(myctrl ${DIR_SRCS})
target_link_libraries(myctrl lcm pthread)
