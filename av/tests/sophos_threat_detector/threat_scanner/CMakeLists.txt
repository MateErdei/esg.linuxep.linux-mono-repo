
SophosAddTest(TestThreatScanner
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        ../../common/MemoryAppender.h
        MockSusiWrapper.h
        MockSusiWrapperFactory.h
        TestSusiGlobalHandler.cpp
        TestSusiGlobalCertificateFunctions.cpp
        TestSusiLogger.cpp
        TestSusiResponseParsing.cpp
        TestThreatScanner.cpp
        TestSusiScannerFactory.cpp
        TestSusiWrapperFactory.cpp
        PROJECTS threat_scanner
        INC_DIRS ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}/modules
        )
