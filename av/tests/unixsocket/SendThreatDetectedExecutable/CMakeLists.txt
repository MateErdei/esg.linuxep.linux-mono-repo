add_executable(SendThreatDetectedEvent
        ../TestClient.h
        SendThreatDetectedEvent.cpp)

target_include_directories(SendThreatDetectedEvent PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR} ${pluginapiinclude})

target_link_libraries(SendThreatDetectedEvent ${zmqlib} ${pluginapilib} ${log4cpluslib} ${CAPNPROTO_LIBRARIES} scanmessages unixsocket)

SET_TARGET_PROPERTIES(SendThreatDetectedEvent
        PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/plugins/av/lib64")

install(TARGETS SendThreatDetectedEvent
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output
        )

add_dependencies(SendThreatDetectedEvent copy_libs)

add_executable(SendDataToSocket
        ../TestClient.h
        SendDataToSocket.cpp)

target_include_directories(SendDataToSocket PUBLIC .${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR} ${pluginapiinclude})

target_link_libraries(SendDataToSocket ${zmqlib} ${pluginapilib} ${log4cpluslib} ${CAPNPROTO_LIBRARIES} scanmessages unixsocket)

SET_TARGET_PROPERTIES(SendDataToSocket
        PROPERTIES BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/plugins/av/lib64")

install(TARGETS SendDataToSocket
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output
        )

add_dependencies(SendDataToSocket copy_libs)
