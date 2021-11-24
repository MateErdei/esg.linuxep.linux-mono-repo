include(GoogleTest)

SophosAddTest(TestSubscriberLib
        MockEventQueuePusher.h
        TestSubscriberLib.cpp
        TestEventQueuePusher.cpp
        PROJECTS subscriberlib eventqueuelib
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest ${protobuflib} heartbeat
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestSubscriberLib copy_libs)
SET_TARGET_PROPERTIES(TestSubscriberLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
