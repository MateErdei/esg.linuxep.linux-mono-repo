add_library(susi_scanner SHARED
        IThreatScanner.h
        IThreatScannerFactory.h
        SusiScanner.cpp
        SusiScanner.h
        SusiScannerFactory.cpp
        SusiScannerFactory.h
        )

target_link_libraries(susi_scanner  PUBLIC
        scanmessages
        datatypes
        )

target_include_directories(susi_scanner PUBLIC ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS susi_scanner
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(susi_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )