CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp)

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


install(TARGETS scanmessages
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
