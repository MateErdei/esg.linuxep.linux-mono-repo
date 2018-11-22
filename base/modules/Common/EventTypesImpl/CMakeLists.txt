include(capnproto/CMakeLists.txt)

add_library(eventtypesimplobject OBJECT
        ../EventTypes/CommonEventData.h
        ../EventTypes/ICredentialEvent.h
        ../EventTypes/IEventType.h
        ../EventTypes/IEventException.h
        ../EventTypes/IEventConverter.h
        CredentialEvent.h
        CredentialEvent.cpp
        EventConverter.h
        EventConverter.cpp
        capnproto/Credentials.capnp.h
        capnproto/Credentials.capnp.c++
        capnproto/Common.capnp.c++
        capnproto/Common.capnp.h
        capnproto/Common.capnp
        capnprotorated)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(eventtypesimpl SHARED $<TARGET_OBJECTS:eventtypesimplobject>)

target_include_directories(eventtypesimpl SYSTEM BEFORE PUBLIC "${CAPNPROTO_INCLUDE_DIR}")

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

add_dependencies(eventtypesimplobject capnproto_definitions)