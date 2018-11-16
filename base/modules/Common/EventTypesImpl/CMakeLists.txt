include(capnproto/CMakeLists.txt)

add_library(eventtypesimplobject OBJECT
        EventTypesImpl.cpp
        EventTypesImpl.h)


add_library(eventtypesimpl SHARED $<TARGET_OBJECTS:eventtypesimplobject>)

SET_TARGET_PROPERTIES( eventtypesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventtypesimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(eventtypesimplobject capnproto_definitions)