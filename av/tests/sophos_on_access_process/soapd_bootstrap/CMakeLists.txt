SophosAddTest(TestSoapdBootstrap
        ../../common/LogInitializedTests.cpp
        TestOnAccessConfigurationUtils.cpp
        TestOnAccessServiceCallback.cpp
        TestSoapdBootstrap.cpp
        PROJECTS soapdbootstrap
        LIBS ${testhelperslib}
        INC_DIRS ${testhelpersinclude}
        )