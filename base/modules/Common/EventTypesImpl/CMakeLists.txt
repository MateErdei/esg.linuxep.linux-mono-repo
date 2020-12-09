CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        capnproto/Common.capnp
        capnproto/PortScanning.capnp
        capnproto/Credentials.capnp
        capnproto/Process.capnp)

add_library(eventtypesimplobject OBJECT
        ../EventTypes/CommonEventData.h
        ../EventTypes/IEventType.h
        ../EventTypes/IEventException.h
        ../EventTypes/IEventConverter.h
        ../EventTypes/CredentialEvent.h
        ../EventTypes/CredentialEvent.cpp
        ../EventTypes/PortScanningEvent.cpp
        ../EventTypes/PortScanningEvent.h
        ../EventTypes/ProcessEvent.h
        ../EventTypes/ProcessEvent.cpp
        ../EventTypes/SophosString.h
        ../EventTypes/SophosString.cpp
        EventConverter.h
        EventConverter.cpp
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        ../EventTypes/EventStrings.h)


target_include_directories(eventtypesimplobject PUBLIC "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR})
add_library(eventtypesimpl SHARED $<TARGET_OBJECTS:eventtypesimplobject>)


target_link_libraries(eventtypesimpl
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a")

SET_TARGET_PROPERTIES(eventtypesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventtypesimpl
        LIBRARY DESTINATION files/base/lib64)

