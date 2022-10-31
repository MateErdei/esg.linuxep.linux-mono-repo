include(GoogleTest)

SophosAddTest(TestThreatReporter
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestReloader.cpp
        TestThreatReporter.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        LIBS pthread
        )
SET_TARGET_PROPERTIES(TestThreatReporter
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

SophosAddTest(TestShutdownTimer
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestShutdownTimer.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        LIBS pthread
        )
SET_TARGET_PROPERTIES(TestShutdownTimer
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

SophosAddTest(TestSophosThreatDetectorMain
        ../../common/LogInitializedTests.cpp
        MockShutdownTimer.h
        MockThreatDetectorResources.h
        MockThreatReporter.h
        TestSophosThreatDetectorMain.cpp
        PROJECTS sophosthreatdetectorimpl common
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS ${log4cpluslib} pthread
        )
SET_TARGET_PROPERTIES(TestSophosThreatDetectorMain
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )