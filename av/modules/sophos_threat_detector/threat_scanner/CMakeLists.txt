add_library(threat_scanner SHARED
        IThreatScanner.h
        IThreatScannerFactory.h
        FakeSusiScanner.cpp
        FakeSusiScanner.h
        FakeSusiScannerFactory.cpp
        FakeSusiScannerFactory.h
        )

target_link_libraries(threat_scanner  PUBLIC
        scanmessages
        datatypes
        )

target_include_directories(threat_scanner PUBLIC ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS threat_scanner
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(threat_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )