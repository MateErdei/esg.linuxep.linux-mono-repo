
add_subdirectory(pluginutils)

SophosAddTest(TestCommon
        ../common/LogInitializedTests.cpp
        TestCentralEnums.cpp
        TestExclusion.cpp
        TestNotifyPipeSleeper.cpp
        TestPathUtils.cpp
        TestPidLockFile.cpp
        TestSaferStrerror.cpp
        TestStatusFile.cpp
        TestStringUtils.cpp
        TestSusiSettings.cpp
        signals/TestSigHupMonitor.cpp
        signals/TestSigIntMonitor.cpp
        signals/TestSigTermMonitor.cpp
        signals/TestSignalHandlerBase.cpp
        signals/TestSignalHandlerTemplate.cpp
        signals/TestSigUSR1Monitor.cpp
        PROJECTS common
        LIBS ${testhelperslib} ${log4cpluslib}
        INC_DIRS ${testhelpersinclude} ${JSON_INPUT}
        )
