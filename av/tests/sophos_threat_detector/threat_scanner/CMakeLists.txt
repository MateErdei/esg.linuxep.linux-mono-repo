
include(GoogleTest)

SophosAddTest(TestThreatScanner
        ../../common/LogInitializedTests.cpp
        ../../common/LogInitializedTests.h
        ../../common/MemoryAppender.h
        MockSusiScannerFactory.h
        MockSusiWrapper.h
        MockSusiWrapperFactory.h
        TestSusiGlobalHandler.cpp
        TestSusiGlobalCertificateFunctions.cpp
        TestSusiLogger.cpp
        TestSusiResponseParsing.cpp
        TestThreatScanner.cpp
        TestSusiScannerFactory.cpp
        TestSusiWrapperFactory.cpp
        PROJECTS threat_scanner common
        INC_DIRS ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        LIBS pthread
        )
SET_TARGET_PROPERTIES(TestThreatScanner
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )