include(GoogleTest)

SophosAddTest(TestSubscriberLib
        TestSubscriberLib.cpp
        MockZmqContext.h
        PROJECTS subscriberlib
        LIBS ${log4cpluslib} ${testhelperslib} rt
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}

)

add_dependencies(TestSubscriberLib copy_libs)
SET_TARGET_PROPERTIES(TestSubscriberLib
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

