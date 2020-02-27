
add_executable(fd_transfer_client
        fd_transfer_client.cpp)
target_link_libraries(fd_transfer_client  PUBLIC unixsocket)
target_include_directories(fd_transfer_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(fd_transfer_server
        fd_transfer_server.cpp)
target_link_libraries(fd_transfer_server  PUBLIC unixsocket)
target_include_directories(fd_transfer_server PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatReporterClientExecutable
        ThreatReporterTestExecutable/threatReporterClient.cpp)
target_link_libraries(threatReporterClientExecutable  PUBLIC unixsocket)
target_include_directories(threatReporterClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatReporterServerExecutable
        ThreatReporterTestExecutable/threatReporterServer.cpp)
target_link_libraries(threatReporterServerExecutable  PUBLIC unixsocket)
target_include_directories(threatReporterServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

