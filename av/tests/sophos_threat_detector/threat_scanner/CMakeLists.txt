
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
        TestSusiRuntimeConfig.cpp
        TestSusiScanner.cpp
        TestSusiScannerFactory.cpp
        TestSusiScanResultJsonParser.cpp
        TestSusiWrapperFactory.cpp
        TestThreatDetectedBuilder.cpp
        TestThreatScanner.cpp
        TestUnitScanner.cpp
        PROJECTS threat_scanner
        INC_DIRS ${testhelpersinclude}
        ${JSON_SINGLE_INPUT}
        LIBS ${testhelperslib}
        )

add_executable(AllowListTester
        AllowListTester.cpp
        )
target_link_libraries(AllowListTester  PUBLIC sophosthreatdetectorimpl ${log4cpluslib})
target_include_directories(AllowListTester PUBLIC ${CMAKE_SOURCE_DIR}/modules)
SET_TARGET_PROPERTIES( AllowListTester
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )
