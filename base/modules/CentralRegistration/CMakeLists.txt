cmake_minimum_required(VERSION 3.8)

add_library(centralregistrationimpl SHARED
        Logger.cpp
        Logger.h
        Main.h
        Main.cpp
        IAdapter.h
        AgentAdapter.h
        AgentAdapter.cpp
        CentralRegistration.h
        CentralRegistration.cpp
        )

target_include_directories(centralregistrationimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
#include_directories(centralregistrationimpl BEFORE ${SUL_INPUT}/include)

target_link_libraries(centralregistrationimpl PUBLIC osutilitiesimpl ${LOG4CPLUS_LIBRARY} $<TARGET_OBJECTS:loggingobject>)

SET_TARGET_PROPERTIES( centralregistrationimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS centralregistrationimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(centralregistrationimpl copy_libs)
