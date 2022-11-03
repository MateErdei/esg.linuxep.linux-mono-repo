SophosAddTest(TestFanotifyHandler
        ../../common/LogInitializedTests.cpp
        MockFanotifyHandler.h
        TestEventReaderThread.cpp
        TestFanotifyHandler.cpp
        TestProcUtils.cpp
        PROJECTS fanotifyhandler
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        )
