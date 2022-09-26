include(GoogleTest)

SophosAddTest(TestAVScannerImpl
        ../../common/LogInitializedTests.cpp
        MockMountPoint.h
        TestCommandLineScanRunner.cpp
        TestNamedScanRunner.cpp
        TestOptions.cpp
        TestScanCallbackImpl.cpp
        TestScanClient.cpp
        TestTimeDuration.cpp
        TestLogger.cpp
        TestBaseFileWalkCallbacks.cpp
        TestClientSocketWrapper.cpp
        PROJECTS avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        LIBS pthread
        )

SET_TARGET_PROPERTIES(TestAVScannerImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

