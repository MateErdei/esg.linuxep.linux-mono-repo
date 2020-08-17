include(GoogleTest)

SophosAddTest(TestMountInfoImpl
        ../../common/LogInitializedTests.cpp
        TestDeviceUtil.cpp
        TestMounts.cpp
        TestSystemPaths.cpp
        PROJECTS mountinfoimpl
        LIBS ${pluginapilib}
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}
        )

SET_TARGET_PROPERTIES(TestMountInfoImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
