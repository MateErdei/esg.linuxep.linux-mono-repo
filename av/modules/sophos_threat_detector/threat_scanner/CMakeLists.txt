add_library(threat_scanner SHARED
        IThreatScanner.h
        IThreatScannerFactory.h
        FakeSusiScanner.cpp
        FakeSusiScanner.h
        FakeSusiScannerFactory.cpp
        FakeSusiScannerFactory.h
        Logger.h
        Logger.cpp
        SusiGlobalHandler.h
        SusiGlobalHandler.cpp
        SusiScanner.h
        SusiScanner.cpp
        SusiWrapper.h
        SusiWrapper.cpp
        SusiScannerFactory.cpp SusiScannerFactory.h)

find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

target_link_libraries(threat_scanner
        PUBLIC scanmessages datatypes $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        PRIVATE ${SUSI_LIB}
        )

target_include_directories(threat_scanner
        PUBLIC ${CMAKE_SOURCE_DIR}/modules
        PRIVATE ${susi_include})

install(TARGETS threat_scanner
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(threat_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:$ORIGIN/../chroot/susi/distribution_version"
        )