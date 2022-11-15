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

