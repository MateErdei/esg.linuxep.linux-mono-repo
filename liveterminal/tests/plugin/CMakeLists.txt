include(GoogleTest)

SophosAddTest(TestPlugin
        TestPlugin.cpp
        TestPluginUtils.cpp
        TestPluginAdapter.cpp
        TestPluginWithMockFileSystem.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} rt pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
)

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
)