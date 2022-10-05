
add_subdirectory(ObfuscationImpl)

include(GoogleTest)

SophosAddTest(TestPlugin
        ../common/LogInitializedTests.cpp
        TestPolicyProcessor.cpp
        TestPlugin.cpp
        TestPluginAdapter.cpp
        TestPluginCallback.cpp
        TestPolicyWaiter.cpp
        TestSafeStoreQueue.cpp
        TestStringUtils.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} rt pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
)

add_dependencies(TestPlugin copy_libs unixsocket)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )