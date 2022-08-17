include(GoogleTest)

SophosAddTest(TestFANotifyHandler
        ../../common/LogInitializedTests.cpp
        TestEventReaderThread.cpp
        TestFANotifyHandler.cpp
        PROJECTS fanotifyhandler
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS pthread
        )

SET_TARGET_PROPERTIES(TestFANotifyHandler
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
