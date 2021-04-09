
add_subdirectory(ObfuscationImpl)

include(GoogleTest)

SophosAddTest(TestPlugin
        ../common/LogInitializedTests.cpp
        MockFileSystem.cpp
        MockFileSystem.h
        TestPolicyProcessor.cpp
        TestPlugin.cpp
        TestPluginAdapter.cpp
        TestPluginCallback.cpp
        TestPubSub.cpp
        TestStringUtils.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} rt
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
)

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )