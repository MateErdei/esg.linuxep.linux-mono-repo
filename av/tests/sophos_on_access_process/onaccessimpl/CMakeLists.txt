SophosAddTest(TestOnAccessImpl
        ../../common/LogInitializedTests.cpp
        OnAccessImplMemoryAppenderUsingTests.h
        TestClientSocketWrapper.cpp
        TestScanRequestHandler.cpp
        TestScanRequestQueue.cpp
        PROJECTS onaccessimpl
        INC_DIRS ${testhelpersinclude}
        )

SophosAddTest(TestOnAccessTelemetryUtility
        ../../common/LogInitializedTests.cpp
        OnAccessImplMemoryAppenderUsingTests.h
        TestOnAccessTelemetryUtility.cpp
        PROJECTS onaccessimpl
        INC_DIRS ${testhelpersinclude}
)


SophosAddTest(TestOnAccessScanRequest
        ../../common/LogInitializedTests.cpp
        OnAccessImplMemoryAppenderUsingTests.h
        TestOnAccessScanRequest.cpp
        PROJECTS onaccessimpl
        INC_DIRS ${testhelpersinclude}
)

