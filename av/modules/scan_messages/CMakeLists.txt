CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp
        NamedScan.capnp
        ThreatDetected.capnp)

add_library(scanmessages SHARED
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        Logger.cpp
        Logger.h
        ScanRequest.cpp
        ScanRequest.h
        ScanResponse.cpp
        ScanResponse.h
        ScanType.h
        ClientScanRequest.cpp
        ClientScanRequest.h
        ServerThreatDetected.cpp
        ServerThreatDetected.h
        ThreatDetected.cpp
        ThreatDetected.h)

target_include_directories(scanmessages PUBLIC
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        )

target_link_libraries(scanmessages datatypes ${log4cpluslib} ${pluginapilib})

SET_TARGET_PROPERTIES(scanmessages
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64")


install(TARGETS scanmessages
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
