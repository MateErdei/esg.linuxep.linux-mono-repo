CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        capnproto/Common.capnp
        capnproto/PortScanning.capnp
        capnproto/Credentials.capnp)

add_library(eventtypesimplobject OBJECT
        ../EventTypes/CommonEventData.h
        ../EventTypes/ICredentialEvent.h
        ../EventTypes/IEventType.h
        ../EventTypes/IEventException.h
        ../EventTypes/IEventConverter.h
        ../EventTypes/IPortScanningEvent.h
        CredentialEvent.h
        CredentialEvent.cpp
        PortScanningEvent.cpp
        PortScanningEvent.h
        EventConverter.h
        EventConverter.cpp
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        )


target_include_directories(eventtypesimplobject PUBLIC "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR})
add_library(eventtypesimpl SHARED $<TARGET_OBJECTS:eventtypesimplobject>)


target_link_libraries(eventtypesimpl
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a")

SET_TARGET_PROPERTIES( eventtypesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventtypesimpl
        LIBRARY DESTINATION files/base/lib64)

