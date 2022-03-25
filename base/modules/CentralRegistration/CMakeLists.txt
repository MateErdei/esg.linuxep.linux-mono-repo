cmake_minimum_required(VERSION 3.8)

add_library(centralregistrationimpl SHARED
        Logger.cpp
        Logger.h
        )


#include_directories(centralregistrationimpl BEFORE ${SUL_INPUT}/include)

target_link_libraries(centralregistrationimpl PUBLIC osutilitiesimpl)

SET_TARGET_PROPERTIES( centralregistrationimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS centralregistrationimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(centralregistrationimpl copy_libs)
