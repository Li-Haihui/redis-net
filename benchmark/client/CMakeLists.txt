message("NET_ROOT_DIR: ${NET_ROOT_DIR}")

add_executable(net_tcpClient tcpClient.cpp)
target_include_directories(net_tcpClient PRIVATE
    ${NET_ROOT_DIR}/network/include
    ${NET_ROOT_DIR}/taskwork/include)
target_compile_options(net_tcpClient PRIVATE -fPIC -g -ggdb -std=c++14)
target_link_libraries(net_tcpClient PUBLIC pthread network taskwork)

add_executable(net_unixClient unixClient.cpp)
target_include_directories(net_unixClient PRIVATE
    ${NET_ROOT_DIR}/network/include
    ${NET_ROOT_DIR}/taskwork/include)
target_compile_options(net_unixClient PRIVATE -fPIC -g -ggdb)
target_link_libraries(net_unixClient PUBLIC pthread network taskwork)