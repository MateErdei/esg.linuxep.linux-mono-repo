include(GoogleTest)

SophosAddTest(TestAVScannerImpl
        ../../common/LogInitializedTests.cpp
        MockMountPoint.h
        SetupLogging.cpp
        TestCommandLineScanRunner.cpp
        TestDeviceUtil.cpp
        TestExclusion.cpp
        TestMounts.cpp
        TestNamedScanRunner.cpp
        TestOptions.cpp
        TestScanClient.cpp
        TestSystemPaths.cpp
        PROJECTS avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

SET_TARGET_PROPERTIES(TestAVScannerImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

