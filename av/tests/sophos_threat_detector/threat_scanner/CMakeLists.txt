
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


find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

add_executable(explore_susi_install
        explore_susi_install.cpp
)
target_link_libraries(explore_susi_install PUBLIC
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${SUSI_LIB}
)
target_include_directories(explore_susi_install
        PUBLIC
        ${susi_include}
)
SET_TARGET_PROPERTIES( explore_susi_install
        PROPERTIES
        BUILD_RPATH "$ORIGIN:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/plugins/av/chroot/lib64"
)

add_executable(explore_susi_install_c
        explore_susi_install.c
)
target_link_libraries(explore_susi_install_c PUBLIC
        ${SUSI_LIB}
)
target_include_directories(explore_susi_install_c
        PUBLIC
        /usr/include/x86_64-linux-gnu
        ${susi_include}
)
SET_TARGET_PROPERTIES(explore_susi_install_c
        PROPERTIES
        BUILD_RPATH "$ORIGIN:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/plugins/av/chroot/lib64"
)
