add_executable(SendThreatDetectedEvent
        ../TestClient.h
        SendThreatDetectedEvent.cpp)

target_include_directories(SendThreatDetectedEvent PUBLIC ../../../ ${pluginapiinclude})

target_link_libraries(SendThreatDetectedEvent ${zmqlib} ${pluginapilib} ${log4cpluslib} ${CAPNPROTO_LIBRARIES} scanmessages unixsocket)

SET_TARGET_PROPERTIES(SendThreatDetectedEvent
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/plugins/av/lib64;${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64")

install(TARGETS SendThreatDetectedEvent
        DESTINATION ../manualtests
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )

