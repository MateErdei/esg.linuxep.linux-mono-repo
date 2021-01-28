include(GoogleTest)

SophosAddTest(TestThreatReporter
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestThreatReporter.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestThreatReporter
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
