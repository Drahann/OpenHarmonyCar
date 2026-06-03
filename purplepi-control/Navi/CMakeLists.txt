# 设置CMake的最小版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(Navigation VERSION 1.0)

# 设置编译选项
set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_FLAGS "-O0 -g")
add_compile_options(-g -w)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加头文件搜索路径
include_directories(udp map math planner particlefilter gaussestimator lcmtype_dir lcmtype_dir/lcm opencv)

# 添加库文件路径
link_directories(
    ${CMAKE_SOURCE_DIR}/lcmlib64
)

# 添加库文件搜索路径
# link_directories(lcmlib64 lcmlib zliblib)

# 指定链接时需要的库
set(DEPEND_LIBS pthread lcm m z)

# 自动将当前目录下的所有源文件添加到变量 DIR_SRCS 中
# aux_source_directory(. DIR_SRCS)

set(SRC_FILES
    udp/UdpClient.cpp
    map/Attributes.cpp
    map/ContourExtractor.cpp
    map/Graph.cpp
    map/Gridmap.cpp
    map/GNode.cpp
    map/GXYTNode.cpp
    map/MapServer.cpp
    map/MultiResolutionScanMatcher.cpp
    map/ScanMatcher.cpp
    map/UnionFindSimple.cpp
    math/LinAlg.cpp
    math/MathUtil.cpp
    math/mymath.cpp
    planner/GridMapRenderer.cpp
    planner/IntMaxHeap.cpp
    planner/PathPlanner.cpp
    planner/WaveFront.cpp
    NaviInterface.cpp
    lcmtype_dir/laser_t.cpp
    lcmtype_dir/path_t.cpp
    lcmtype_dir/pose_t.cpp
    lcmtype_dir/robot_control_t.cpp
    lcmtype_dir/grid_map_t.cpp
    gaussestimator/CholeskyDecomposition.cpp
    gaussestimator/CMatrix.cpp
    gaussestimator/CMultiGauss.cpp
    gaussestimator/CSRAllocator.cpp
    gaussestimator/LUDecomposition.cpp
    gaussestimator/MultiGaussionEstimator.cpp
    gaussestimator/Vec.cpp
    planner/Astarplanner.cpp
    planner/DStarSearch.cpp
    planner/AStarSearch.cpp
    planner/BFSSearch.cpp
    planner/ZigzagCoverage.cpp
    planner/STCCoverage.cpp
    planner/FullPathCoverage.cpp
    planner/CleanPlanner.cpp
    planner/ant.cpp
    particlefilter/ParticleFilter.cpp
    particlefilter/KLD.cpp
    main.cpp
)

# 添加可执行文件
add_executable(navigation ${SRC_FILES})

# 链接库文件
target_link_libraries(navigation ${DEPEND_LIBS})
