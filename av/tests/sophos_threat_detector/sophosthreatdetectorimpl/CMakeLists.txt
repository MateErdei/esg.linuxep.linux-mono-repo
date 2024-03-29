SophosAddTest(TestThreatReporter
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestReloader.cpp
        TestSafeStoreRescanWorker.cpp
        TestThreatReporter.cpp
        PROJECTS sophosthreatdetectorimpl
        LIBS ${testhelperslib}
        INC_DIRS ${testhelpersinclude}
        )

SophosAddTest(TestShutdownTimer
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestShutdownTimer.cpp
        PROJECTS sophosthreatdetectorimpl
        )

SophosAddTest(TestProcessForceExitTimer
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestProcessForceExitTimer.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS ${testhelpersinclude}
        )

SophosAddTest(TestSophosThreatDetectorMain
        ../../common/LogInitializedTests.cpp
        MockMetadataRescanServerSocket.h
        MockReloader.h
        MockSafeStoreRescanWorker.h
        MockScanningServerSocket.h
        MockShutdownTimer.h
        MockThreatDetectorControlCallbacks.h
        MockThreatDetectorResources.h
        MockThreatReporter.h
        MockUpdateCompleteServerSocket.h
        TestSophosThreatDetectorMain.cpp
        TestProxySettings.cpp
        PROJECTS sophosthreatdetectorimpl common
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        ${testhelpersinclude}
        LIBS ${log4cpluslib} pthread ${testhelperslib}
        )
SET_TARGET_PROPERTIES(TestSophosThreatDetectorMain
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )