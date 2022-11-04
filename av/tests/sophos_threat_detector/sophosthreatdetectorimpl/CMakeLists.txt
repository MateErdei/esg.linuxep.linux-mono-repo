SophosAddTest(TestThreatReporter
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestReloader.cpp
        TestThreatReporter.cpp
        PROJECTS sophosthreatdetectorimpl
        )

SophosAddTest(TestShutdownTimer
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        TestShutdownTimer.cpp
        PROJECTS sophosthreatdetectorimpl
        )

