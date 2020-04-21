message("testheaderinclude: ${testhelpersinclude}")

SophosAddTest(TestQueryRunner
        ParallelQueryProcessorTests.cpp
        QueryRunnerImplTests.cpp
        PROJECTS queryrunnerimpl
        LIBS ${pluginapilib} ${log4cpluslib} ${testhelperslib} gmock gtest
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestOsqueryProcessor copy_libs)


SET_TARGET_PROPERTIES(TestQueryRunner
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )        