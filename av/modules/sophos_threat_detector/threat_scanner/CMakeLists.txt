add_library(threat_scanner SHARED
        Logger.h
        Logger.cpp
        ISusiWrapper.h
        ISusiWrapperFactory.h
        IThreatReporter.h
        IThreatScanner.h
        IThreatScannerFactory.h
        SusiCertificateFunctions.h
        SusiCertificateFunctions.cpp
        SusiGlobalHandler.h
        SusiGlobalHandler.cpp
        SusiLogger.h
        SusiLogger.cpp
        SusiScanner.h
        SusiScanner.cpp
        SusiScannerFactory.h
        SusiScannerFactory.cpp
        SusiWrapper.h
        SusiWrapper.cpp
        SusiWrapperFactory.h
        SusiWrapperFactory.cpp
        ScannerInfo.h
        ScannerInfo.cpp
        ThrowIfNotOk.h )

find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

get_filename_component(ABS_SUSI_LIB ${SUSI_LIB} REALPATH)

target_link_libraries(threat_scanner PUBLIC
        datatypes
        obfuscationimpl
        scanmessages
        unixsocket
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${SUSI_LIB}
        )

target_include_directories(threat_scanner
        PUBLIC ${CMAKE_SOURCE_DIR}/modules ${susi_include})

install(TARGETS threat_scanner
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(threat_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )

install(FILES ${ABS_SUSI_LIB}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

set(SUSI_INSTALL_DIR files/plugins/${PLUGIN_NAME}/chroot/susi)
install(DIRECTORY ${SUSI_SDDS_PATH}/ DESTINATION ${SUSI_INSTALL_DIR})

