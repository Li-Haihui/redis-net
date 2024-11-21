cmake_minimum_required(VERSION 3.13)
#project(net-benchmark)

# 添加 C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC_FILES "main.c")


add_executable(net_benchmark
    ${SRC_FILES}
)

target_include_directories(net_benchmark PRIVATE
    ../include
)

target_compile_options(net_benchmark PRIVATE -fPIC -g -ggdb -std=c99)

#target_link_directories(net_benchmark PRIVATE
#
#)

target_link_libraries(net_benchmark PUBLIC
    pthread
    net
)


#install(TARGETS net_benchmark)