include(GoogleTest)

SophosAddTest(TestSubscriberLib
        MockEventQueuePusher.h
        TestSubscriberLib.cpp
        TestEventQueuePusher.cpp
        ${CMAKE_SOURCE_DIR}/modules/Heartbeat/HeartbeatPinger.cpp
        PROJECTS subscriberlib eventqueuelib
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest ${protobuflib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestSubscriberLib copy_libs)
SET_TARGET_PROPERTIES(TestSubscriberLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
