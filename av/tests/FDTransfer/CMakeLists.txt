
add_executable(fd_transfer_client
        fd_transfer_client.cpp)
target_link_libraries(fd_transfer_client  PUBLIC unixsocket)
target_include_directories(fd_transfer_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(fd_transfer_server
        fd_transfer_server.cpp)
target_link_libraries(fd_transfer_server  PUBLIC unixsocket threat_scanner)
target_include_directories(fd_transfer_server PUBLIC  ${CMAKE_SOURCE_DIR}/modules)

