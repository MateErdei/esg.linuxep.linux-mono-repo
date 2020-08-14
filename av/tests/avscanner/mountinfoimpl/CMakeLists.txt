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

add_executable(PrintMounts
        PrintMounts.cpp
        )
target_link_libraries(PrintMounts PRIVATE
        mountinfoimpl
        ${pluginapilib}
        )
target_include_directories(PrintMounts PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES(PrintMounts
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
        )
