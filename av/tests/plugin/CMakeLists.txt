include(GoogleTest)

SophosAddTest(TestPlugin
        TestPlugin.cpp
        TestPubSub.cpp
        PROJECTS pluginimpl
        LIBS ${testhelpers} ${pluginapilib} rt
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}
)

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

# example with third party dependencies
#SophosAddTest(TestPlugin
#        TestPlugin.cpp
#        PROJECTS pluginimpl
#        LIBS ${log4cpluslib} ${pluginapilib}
#        INC_DIRS ${pluginapiinclude} )

add_dependencies(TestPlugin copy_libs)

SET_TARGET_PROPERTIES(TestPlugin
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )