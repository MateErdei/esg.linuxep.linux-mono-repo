SophosAddTest(TestThreatReporter
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestReloader.cpp
        TestThreatReporter.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

SophosAddTest(TestShutdownTimer
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestShutdownTimer.cpp
        PROJECTS sophosthreatdetectorimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )

