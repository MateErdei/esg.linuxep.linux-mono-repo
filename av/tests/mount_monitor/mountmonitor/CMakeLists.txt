SophosAddTest(TestMountMonitor
        ../../common/LogInitializedTests.cpp
        TestMountMonitor.cpp
        PROJECTS mountmonitor
        LIBS ${log4cpluslib}
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        )

