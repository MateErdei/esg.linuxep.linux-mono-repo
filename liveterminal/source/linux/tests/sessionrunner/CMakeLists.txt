message("testheaderinclude: ${testhelpersinclude}")
include(GoogleTest)

SophosAddTest(TestSessionRunner
        ParallelSessionProcessorTests.cpp
        SessionRunnerImplTests.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${log4cpluslib} ${testhelperslib} pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
        )

add_dependencies(TestSessionRunner copy_libs)

SET_TARGET_PROPERTIES(TestSessionRunner
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )