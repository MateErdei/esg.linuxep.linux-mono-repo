
add_library(soapdbootstrap SHARED
        Logger.cpp
        Logger.h
        OnAccessMountConfig.h
        SoapdBootstrap.cpp
        SoapdBootstrap.h
        )

target_link_libraries(soapdbootstrap  PUBLIC
        common
        mountinfoimpl
        datatypes
        unixsocket
        onAccessConfigMonitor
        )

install(TARGETS soapdbootstrap
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(soapdbootstrap
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
