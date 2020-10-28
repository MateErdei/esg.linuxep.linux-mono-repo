
SophosAddTest(TestPlugin
        TestQueueTask.cpp
        TestOsqueryDataManager.cpp
        TestOsqueryConfigurator.cpp
        TestPluginAdapter.cpp
        TestPluginUtils.cpp
        ALCPoliciesExample.h
        ALCPoliciesExample.cpp
        OsqueryTelemetryTests.cpp
        PROJECTS pluginimpl OsqueryExtensions
        LIBS ${pluginapilib} ${testhelperslib} rt dl ${OPENSSL_LIBRARY} ${OSQUERY_GEN_LIBRARY} ${GFLAG_LIBRARY}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
)

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )