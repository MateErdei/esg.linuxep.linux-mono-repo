CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        ScanRequest.capnp
        ScanResponse.capnp)

add_library(scanmessagesobject OBJECT
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        )

target_include_directories(scanmessagesobject PUBLIC "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR})
add_library(scanmessages SHARED $<TARGET_OBJECTS:scanmessagesobject>)
