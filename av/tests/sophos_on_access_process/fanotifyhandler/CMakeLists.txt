include(GoogleTest)

SophosAddTest(TestFanotifyHandler
        ../../common/LogInitializedTests.cpp
        MockFanotifyHandler.h
        TestEventReaderThread.cpp
        TestFanotifyHandler.cpp
        TestProcUtils.cpp
        PROJECTS fanotifyhandler
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS pthread
        )

SET_TARGET_PROPERTIES(TestFanotifyHandler
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
