
add_executable(fd_transfer_client
        fd_transfer_client.cpp)
target_link_libraries(fd_transfer_client  PUBLIC unixsocket
        datatypes
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
target_include_directories(fd_transfer_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(fd_transfer_server
        fd_transfer_server.cpp)
target_link_libraries(fd_transfer_server  PUBLIC unixsocket
        datatypes
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
target_include_directories(fd_transfer_server PUBLIC  ${CMAKE_SOURCE_DIR}/modules)

