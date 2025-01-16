cmake_minimum_required(VERSION 3.13)

project(net)
set(NET_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(network)
add_subdirectory(taskwork)
add_subdirectory(benchmark)
