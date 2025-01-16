cmake_minimum_required(VERSION 3.13)
project(network)

# 添加 C++14 标准
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC_FILES
    src/ae.c
    src/anet.c
)

set(MULTILAYER_SUP "epoll") # 设置默认值为epoll

if(MULTILAYER_SUP STREQUAL "evport")
    add_definitions(-DHAVE_EVPORT)
    file(GLOB_RECURSE MULTILAYER_SUP_FILES "src/ae_evport.c")
elseif(MULTILAYER_SUP STREQUAL "epoll")
    add_definitions(-DHAVE_EPOLL)
    file(GLOB_RECURSE MULTILAYER_SUP_FILES "src/ae_epoll.c")
elseif(MULTILAYER_SUP STREQUAL "kqueue")
    add_definitions(-DHAVE_KQUEUE)
    file(GLOB_RECURSE MULTILAYER_SUP_FILES "src/ae_kqueue.c")
else()
    add_definitions(-DHAVE_EPOLL)
    file(GLOB_RECURSE MULTILAYER_SUP_FILES "src/ae_select.c")
endif()

add_library(network SHARED ${SRC_FILES} ${MULTILAYER_SUP_FILES})

target_include_directories(network PRIVATE
    ./include
)

target_compile_options(network PRIVATE -fPIC -g -ggdb)

#target_link_directories(net PRIVATE
#
#)

target_link_libraries(network PUBLIC
    pthread
)


