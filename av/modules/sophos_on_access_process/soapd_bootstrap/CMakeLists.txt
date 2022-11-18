
add_library(soapdbootstrap SHARED
        OnAccessConfigurationUtils.cpp
        OnAccessConfigurationUtils.h
        OnAccessProcesControlCallbacks.cpp
        OnAccessProcesControlCallbacks.h
        OnAccessServiceCallback.cpp
        OnAccessServiceCallback.h
        OnAccessTelemetryUtility.cpp
        OnAccessTelemetryUtility.h
        Logger.cpp
        Logger.h
        SoapdBootstrap.cpp
        SoapdBootstrap.h
        )

target_include_directories(soapdbootstrap PUBLIC ${CMAKE_SOURCE_DIR}/products/distribution/include)

target_link_libraries(soapdbootstrap  PUBLIC
        common
        datatypes
        fanotifyhandler
        mountmonitor
        unixsocket
        )

install(TARGETS soapdbootstrap
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(soapdbootstrap
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
