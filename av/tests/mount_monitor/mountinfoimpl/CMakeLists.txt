SophosAddTest(TestMountInfoImpl
        ../../common/LogInitializedTests.cpp
        TestDeviceUtil.cpp
        TestMounts.cpp
        TestSystemPaths.cpp
        PROJECTS mountinfoimpl
        LIBS ${STD_FILESYSTEM_IF_REQUIRED}
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}
        )

