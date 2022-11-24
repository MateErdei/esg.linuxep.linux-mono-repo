
SophosAddTest(TestThreatScanner
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        ../../common/MemoryAppender.h
        MockSusiScannerFactory.h
        MockSusiWrapper.h
        MockSusiWrapperFactory.h
        MockSusiApi.h
        TestSusiGlobalCertificateFunctions.cpp
        TestSusiGlobalHandler.cpp
        TestSusiLogger.cpp
        TestSusiResultUtils.cpp
        TestSusiScanner.cpp
        TestSusiScannerFactory.cpp
        TestSusiScanResultJsonParser.cpp
        TestSusiWrapperFactory.cpp
        TestThreatDetectedBuilder.cpp
        TestThreatScanner.cpp
        TestUnitScanner.cpp
        PROJECTS threat_scanner
        INC_DIRS ${testhelpersinclude}
        LIBS ${testhelperslib}
        )
