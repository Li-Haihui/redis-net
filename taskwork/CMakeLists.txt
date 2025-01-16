cmake_minimum_required(VERSION 3.13)
project(taskwork)

# 添加 C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC_FILES
    src/InterruptFlag.cpp
    src/TaskStealQueue.cpp
    src/ThreadPool.cpp
)


add_library(taskwork SHARED ${SRC_FILES} ${MULTILAYER_SUP_FILES})

target_include_directories(taskwork PRIVATE
    ./include
)

target_compile_options(taskwork PRIVATE -fPIC -g -ggdb)

#target_link_directories(net PRIVATE
#
#)

target_link_libraries(taskwork PUBLIC
    pthread
)


