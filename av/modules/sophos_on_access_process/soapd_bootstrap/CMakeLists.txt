
add_library(soapdbootstrap SHARED
        SoapdBootstrap.cpp
        SoapdBootstrap.h
        Logger.cpp
        Logger.h
        )

target_link_libraries(soapdbootstrap  PUBLIC
        common
        datatypes
        unixsocket
        )

install(TARGETS soapdbootstrap
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(soapdbootstrap
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
