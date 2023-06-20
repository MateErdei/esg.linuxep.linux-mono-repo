SophosAddTest(TestScanProcessMonitor
        ../../common/LogInitializedTests.cpp
        TestConfigMonitor.cpp
        TestScanProcessMonitor.cpp
        PROJECTS scanprocessmonitor
        LIBS ${pluginapilib}
        INC_DIRS ${testhelpersinclude}
        )
