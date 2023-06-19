SophosAddTest(TestFanotifyHandler
        ../../common/LogInitializedTests.cpp
        MockFanotifyHandler.h
        TestEventReaderThread.cpp
        TestExclusionCache.cpp
        TestExecutablePathCache.cpp
        TestFanotifyHandler.cpp
        TestProcUtils.cpp
        PROJECTS fanotifyhandler
        LIBS ${testhelperslib}
        INC_DIRS ${testhelpersinclude}
        )
