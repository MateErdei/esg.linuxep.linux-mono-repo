CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp
        MetadataRescan.capnp
        NamedScan.capnp
        ProcessControl.capnp
        QuarantineResponse.capnp
        RestoreReport.capnp
        ThreatDetected.capnp
        )

add_library(scanmessages SHARED
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        ClientScanRequest.cpp
        ClientScanRequest.h
        Logger.cpp
        Logger.h
        MetadataRescan.cpp
        MetadataRescan.h
        ProcessControlDeserialiser.cpp
        ProcessControlDeserialiser.h
        ProcessControlSerialiser.cpp
        ProcessControlSerialiser.h
        QuarantineResponse.cpp
        QuarantineResponse.h
        RestoreReport.h
        ScanRequest.cpp
        ScanRequest.h
        ScanResponse.cpp
        ScanResponse.h
        ScanType.h
        Threat.h
        ThreatDetected.cpp
        ThreatDetected.h
        )

target_include_directories(scanmessages PUBLIC
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        ${JSON_SINGLE_INPUT}
        )

target_link_libraries(scanmessages
        common
        datatypes
        ${log4cpluslib} ${pluginapilib} ${CAPNPROTO_LIBRARIES})

SET_TARGET_PROPERTIES(scanmessages
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")


install(TARGETS scanmessages
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
