add_library(threat_scanner SHARED
        IScanNotification.h
        IUnitScanner.h
        ISusiWrapper.h
        ISusiWrapperFactory.h
        IThreatReporter.h
        IThreatScanner.h
        IThreatScannerFactory.h
        IUpdateCompleteCallback.h
        Logger.h
        Logger.cpp
        ScannerInfo.h
        ScannerInfo.cpp
        ScanResult.h
        SusiCertificateFunctions.h
        SusiCertificateFunctions.cpp
        SusiGlobalHandler.h
        SusiGlobalHandler.cpp
        SusiLogger.h
        SusiLogger.cpp
        SusiResultUtils.h
        SusiResultUtils.cpp
        SusiScanner.h
        SusiScanner.cpp
        SusiScannerFactory.h
        SusiScannerFactory.cpp
        SusiScanResultJsonParser.h
        SusiScanResultJsonParser.cpp
        SusiWrapper.h
        SusiWrapper.cpp
        SusiWrapperFactory.h
        SusiWrapperFactory.cpp
        ThreatDetectedBuilder.h
        ThreatDetectedBuilder.cpp
        ThrowIfNotOk.h
        UnitScanner.h
        UnitScanner.cpp
        )

find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})
find_library(UPDATER_LIB NAMES updater PATHS ${SUSIPATH})
get_filename_component(ABS_SUSI_LIB ${SUSI_LIB} REALPATH)
get_filename_component(ABS_UPDATER_LIB ${UPDATER_LIB} REALPATH)

target_link_libraries(threat_scanner PUBLIC
        common
        datatypes
        obfuscationimpl
        scanmessages
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

install(FILES ${ABS_SUSI_LIB} ${ABS_UPDATER_LIB}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

set(SUSI_INSTALL_DIR files/plugins/${PLUGIN_NAME}/chroot/susi)
install(DIRECTORY ${SUSI_SDDS_PATH}/ DESTINATION ${SUSI_INSTALL_DIR})

