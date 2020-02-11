CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp
        NamedScan.capnp)

add_library(scanmessages SHARED
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        ScanRequest.cpp
        ScanRequest.h
        ScanResponse.cpp
        ScanResponse.h
        ClientScanRequest.cpp
        ClientScanRequest.h )

target_include_directories(scanmessages PUBLIC
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        )

target_link_libraries(scanmessages datatypes)

SET_TARGET_PROPERTIES(scanmessages
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64")


install(TARGETS scanmessages
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
