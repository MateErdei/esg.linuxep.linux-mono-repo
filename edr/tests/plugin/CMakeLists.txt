
SophosAddTest(TestPlugin
        TestLiveQueryPolicyParser.cpp
        TestQueueTask.cpp
        TestOsqueryDataManager.cpp
        TestOsqueryConfigurator.cpp
        TestPluginAdapter.cpp
        TestPluginUtils.cpp
        OsqueryTelemetryTests.cpp
        TestOsqueryLogStringUtil.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} rt dl
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
)

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )