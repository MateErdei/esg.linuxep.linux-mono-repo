include(capnproto/CMakeLists.txt)

add_library(eventtypesimplobject OBJECT
        ../EventTypes/CommonEventData.h
        ../EventTypes/ICredentialEvent.h
        ../EventTypes/IEventType.h
        ../EventTypes/IEventException.h
        CredentialEvent.h
        CredentialEvent.cpp
        EventConverter.h
        EventConverter.cpp
        capnproto/generated/Credentials.capnp.h
        capnproto/generated/Credentials.capnp.c++)


add_library(eventtypesimpl SHARED $<TARGET_OBJECTS:eventtypesimplobject>)

SET_TARGET_PROPERTIES( eventtypesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventtypesimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(eventtypesimplobject capnproto_definitions)