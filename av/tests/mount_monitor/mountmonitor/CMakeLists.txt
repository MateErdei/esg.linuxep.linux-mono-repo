include(GoogleTest)

SophosAddTest(TestMountMonitor
        ../../common/LogInitializedTests.cpp
        TestMountMonitor.cpp
        PROJECTS mountmonitor
        LIBS pthread
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        )

SET_TARGET_PROPERTIES(TestMountMonitor
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
