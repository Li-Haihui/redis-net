cmake_minimum_required(VERSION 3.13)
#project(net-benchmark)

# 添加 C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("NET_ROOT_DIR: ${NET_ROOT_DIR}")

add_executable(net_tcpService tcpService.cpp)
target_include_directories(net_tcpService PRIVATE
    ${NET_ROOT_DIR}/network/include
    ${NET_ROOT_DIR}/taskwork/include)
target_compile_options(net_tcpService PRIVATE -fPIC -g -ggdb)
target_link_libraries(net_tcpService PUBLIC pthread network taskwork)

add_executable(net_unixService unixService.cpp)
target_include_directories(net_unixService PRIVATE
    ${NET_ROOT_DIR}/network/include
    ${NET_ROOT_DIR}/taskwork/include)
target_compile_options(net_unixService PRIVATE -fPIC -g -ggdb)
target_link_libraries(net_unixService PUBLIC pthread network taskwork)
