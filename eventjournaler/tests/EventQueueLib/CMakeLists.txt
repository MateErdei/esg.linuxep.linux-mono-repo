include(GoogleTest)

SophosAddTest(TestEventQueueLib
        TestEventQueueLib.cpp
        PROJECTS eventqueuelib eventwriterlib subscriberlib
        LIBS ${expatlib} ${log4cpluslib} ${pluginapilib} ${testhelperslib} ${protobuflib} heartbeat pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestEventQueueLib copy_libs)
SET_TARGET_PROPERTIES(TestEventQueueLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

