SophosAddTest(TestOsqueryExtension
        TestResultSender.cpp
        TestSophosServerTable.cpp
        MockQueryContext.h
        PROJECTS OsqueryExtensions pluginimpl
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest pluginimpl
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestOsqueryExtension copy_libs)

SET_TARGET_PROPERTIES(TestOsqueryExtension
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )