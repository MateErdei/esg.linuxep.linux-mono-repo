include(GoogleTest)

SophosAddTest(TestHeartbeat
        TestHeartbeat.cpp
        FakeTimeUtils.cpp
        FakeTimeUtils.h
        ${CMAKE_SOURCE_DIR}/modules/Heartbeat/Heartbeat.h
        ${CMAKE_SOURCE_DIR}/modules/Heartbeat/Heartbeat.cpp
        ${CMAKE_SOURCE_DIR}/modules/Heartbeat/HeartbeatPinger.h
        ${CMAKE_SOURCE_DIR}/modules/Heartbeat/HeartbeatPinger.cpp
        LIBS heartbeat ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest ${protobuflib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude} ${CMAKE_SOURCE_DIR}/modules/Heartbeat
        )

add_dependencies(TestHeartbeat copy_libs)
SET_TARGET_PROPERTIES(TestHeartbeat
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
