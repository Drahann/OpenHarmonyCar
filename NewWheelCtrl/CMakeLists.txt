cmake_minimum_required(VERSION 3.10)
project(WheelCtrl)

# 设置编译参数
add_compile_options(-g -w)
# 添加头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/lcmtype
)
# 添加库文件路径
link_directories(
    ${CMAKE_SOURCE_DIR}/lib/${OHOS_ARCH}
)
# 设置输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(serial)
add_subdirectory(udp2lcm)
add_subdirectory(myctrl)
