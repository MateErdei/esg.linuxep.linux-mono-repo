SophosAddTest(TestScanProcessMonitor
        ../../common/LogInitializedTests.cpp
        TestConfigMonitor.cpp
        TestScanProcessMonitor.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}
        )
