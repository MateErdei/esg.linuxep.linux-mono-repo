
add_executable(SendThreatDetectedEvent
        SendThreatDetectedEvent.cpp)


target_include_directories(SendThreatDetectedEvent PUBLIC ../.. ${pluginapiinclude})


target_link_libraries(SendThreatDetectedEvent ${zmqlib} ${pluginapilib} ${log4cpluslib} ${CAPNPROTO_LIBRARIES})

SET_TARGET_PROPERTIES(SendThreatDetectedEvent
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/plugins/av/lib64;${CMAKE_BINARY_DIR}/libs")


