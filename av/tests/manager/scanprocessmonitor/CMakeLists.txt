include(GoogleTest)

SophosAddTest(TestScanProcessMonitor
        ../../common/LogInitializedTests.cpp
        TestConfigMonitor.cpp
        TestScanProcessMonitor.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} rt pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
        )

add_dependencies(TestScanProcessMonitor copy_libs unixsocket)

SET_TARGET_PROPERTIES(TestScanProcessMonitor
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
