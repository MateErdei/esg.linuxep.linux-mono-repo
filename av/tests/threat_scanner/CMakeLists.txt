
include(GoogleTest)

SophosAddTest(TestThreatScanner
        ../common/LogInitializedTests.cpp
        ../common/LogInitializedTests.h
        MockSusiWrapper.h
        MockSusiWrapperFactory.h
        TestSusiGlobalHandler.cpp
        TestSusiLogger.cpp
        TestSusiResponseParsing.cpp
        TestThreatScanner.cpp
        TestSusiScannerFactory.cpp
        PROJECTS threat_scanner common
        INC_DIRS ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestThreatScanner
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )