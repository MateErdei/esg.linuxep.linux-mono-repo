include(GoogleTest)

SophosAddTest(TestPluginUtils
        ../../common/LogInitializedTests.cpp
        TestThreadRunner.cpp
        PROJECTS common
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}
        ${pluginapiinclude}
        ${testhelpersinclude}
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} pthread rt
        )

add_dependencies(TestPluginUtils
        copy_libs)

SET_TARGET_PROPERTIES(TestPluginUtils
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
