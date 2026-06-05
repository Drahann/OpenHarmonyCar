cmake_minimum_required(VERSION 3.10)
project(lidar_driver)
option(CROSS_COMPILING "Cross compiling" OFF)

set(CMAKE_CXX_STANDARD 11)
# 设置编译选项，忽略所有警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# Add the include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/arch/linux)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/arch/win32)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/arch/macOS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/dataunpacker)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/dataunpacker/unpacker)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/hal)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lcmtype_dir)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add the source files
file(GLOB_RECURSE CXXSRC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dataunpacker/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lcmtype_dir/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_lidar_driver.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/thread.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_crc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_serial_channel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_lidarprotocol_codec.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_async_transceiver.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_tcp_channel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sl_udp_channel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/linux/net_serial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/linux/net_socket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/linux/timer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lidar_driver.cpp"
)


# 添加库文件路径
link_directories(
    ${CMAKE_SOURCE_DIR}/lib/${OHOS_ARCH}
)

# Define the executable
add_executable(${PROJECT_NAME} ${CXXSRC})

# Link the libraries
target_link_libraries(${PROJECT_NAME} lcm pthread)
