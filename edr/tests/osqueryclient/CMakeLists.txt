message("testheaderinclude: ${testhelpersinclude}")

SophosAddTest(TestOsqueryProcessor
        OsqueryProcessorTests.cpp
        PROJECTS osqueryclient
        LIBS ${pluginapilib} ${log4cpluslib} ${testhelperslib} gmock gtest
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )
message("show ${log4cpluslib}")
target_compile_definitions(TestOsqueryProcessor PRIVATE -DOSQUERYBIN="${INPUT}/osquery/usr/bin/osqueryd")

add_dependencies(TestOsqueryProcessor copy_libs)

SET_TARGET_PROPERTIES(TestOsqueryProcessor
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs:${INPUT}/pluginapi/lib64/"
        )