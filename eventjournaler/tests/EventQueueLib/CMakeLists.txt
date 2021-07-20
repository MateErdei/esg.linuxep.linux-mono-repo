include(GoogleTest)

SophosAddTest(TestEventQueueLib
        TestEventQueueLib.cpp
        PROJECTS eventqueuelib eventwriterlib subscriberlib
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest ${protobuflib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestEventQueueLib copy_libs)
SET_TARGET_PROPERTIES(TestEventQueueLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

