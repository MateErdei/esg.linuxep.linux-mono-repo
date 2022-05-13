include(GoogleTest)

SophosAddTest(TestHeartbeat
        TestHeartbeat.cpp
        FakeTimeUtils.cpp
        FakeTimeUtils.h
        LIBS heartbeat ${log4cpluslib} ${pluginapilib} ${testhelperslib} ${protobuflib} pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude} ${CMAKE_SOURCE_DIR}/modules/Heartbeat
        )

add_dependencies(TestHeartbeat copy_libs)
SET_TARGET_PROPERTIES(TestHeartbeat
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
