include(GoogleTest)

SophosAddTest(TestAVScannerImpl
        MockMountPoint.h
        TestCommandLineScanRunner.cpp
        TestDeviceUtil.cpp
        TestExclusion.cpp
        TestMounts.cpp
        TestNamedScanRunner.cpp
        TestOptions.cpp
        PROJECTS avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

SET_TARGET_PROPERTIES(TestAVScannerImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )