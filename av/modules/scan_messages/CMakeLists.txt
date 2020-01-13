CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp)

add_library(scanmessages SHARED
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        ScanRequest.cpp ScanRequest.h
        )

target_include_directories(scanmessages PUBLIC "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR})
