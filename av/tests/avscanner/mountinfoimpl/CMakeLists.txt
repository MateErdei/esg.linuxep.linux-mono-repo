include(GoogleTest)

SophosAddTest(TestMountInfoImpl
        ../../common/LogInitializedTests.cpp
        ../mountinfoimpl/TestDeviceUtil.cpp
        ../mountinfoimpl/TestMounts.cpp
        ../mountinfoimpl/TestSystemPaths.cpp
        PROJECTS mountinfoimpl
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
        )
target_include_directories(PrintMounts PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        ${CMAKE_CURRENT_BINARY_DIR})
