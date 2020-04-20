message("testheaderinclude: ${testhelpersinclude}")

SophosAddTest(TestOsqueryProcessor
        OsqueryProcessorTests.cpp
        OsqueryProcessorTestsWithMock.cpp        
        MockOsqueryClient.h
        PROJECTS osqueryclient
        LIBS ${pluginapilib} ${log4cpluslib} ${testhelperslib} gmock gtest
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )
message("show ${log4cpluslib}")
target_compile_definitions(TestOsqueryProcessor PRIVATE -DOSQUERYBIN="${INPUT}/osquery/usr/local/bin/osqueryd")

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

install(TARGETS TestOsqueryProcessor
        DESTINATION ../componenttests)
