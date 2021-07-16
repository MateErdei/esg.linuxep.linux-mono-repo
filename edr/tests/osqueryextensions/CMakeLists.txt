
SophosAddTest(TestOsqueryExtension
        TestLoggerExtension.cpp
        TestResultSender.cpp
        TestSophosServerTable.cpp
        TestSophosAVDetectionTable.cpp
        MockQueryContext.h
        PROJECTS OsqueryExtensions pluginimpl eventjournalwrapperimpl
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest pluginimpl ${LZMA_LIBRARY} stdc++fs
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude} ${LZMA_INCLUDE_DIR}
        )
target_compile_definitions(TestOsqueryExtension PUBLIC -D_HAS_STD_BYTE)
add_dependencies(TestOsqueryExtension copy_libs)

SET_TARGET_PROPERTIES(TestOsqueryExtension
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
