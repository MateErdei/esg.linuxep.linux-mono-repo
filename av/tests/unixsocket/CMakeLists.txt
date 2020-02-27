
include(GoogleTest)

SophosAddTest(TestSocketUtils
        TestSocketUtils.cpp
        TestThreatReporterSocket.cpp
        PROJECTS unixsocket avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestSocketUtils
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
