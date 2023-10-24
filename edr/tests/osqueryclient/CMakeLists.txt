message("testheaderinclude: ${testhelpersinclude}")

SophosAddTest(TestOsqueryProcessor
        OsqueryProcessorTests.cpp
        OsqueryProcessorTestsWithMock.cpp
        ../EdrCommon/MockOsqueryClient.h
        PROJECTS osqueryclient
        LIBS ${pluginapilib} ${log4cpluslib} ${testhelperslib} ${GFLAG_LIBRARY} pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/.. ${testhelpersinclude}
        )
message("show ${log4cpluslib}")
target_compile_definitions(TestOsqueryProcessor PRIVATE -DOSQUERYBIN="${INPUT}/osquery/opt/osquery/bin/osqueryd")

add_dependencies(TestOsqueryProcessor copy_libs)

SET_TARGET_PROPERTIES(TestOsqueryProcessor
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../files/plugins/edr/lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs:${INPUT}/pluginapi/lib64/"
        )


install(FILES ${INPUT}/pluginapi/tests/lib64/libsharedtesthelpers.so
        DESTINATION ../componenttests
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

