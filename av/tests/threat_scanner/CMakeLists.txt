
include(GoogleTest)

SophosAddTest(TestThreatScanner
        TestThreatScanner.cpp
        PROJECTS threat_scanner
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestThreatScanner
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
