
add_library(soapdbootstrap SHARED
        OnAccessConfigurationUtils.cpp
        OnAccessConfigurationUtils.h
        OnAccessProcesControlCallbacks.cpp
        OnAccessProcesControlCallbacks.h
        OnAccessRunner.cpp
        OnAccessRunner.h
        OnAccessServiceCallback.cpp
        OnAccessServiceCallback.h
        OnAccessServiceImpl.cpp
        OnAccessServiceImpl.h
        OnAccessStatusFile.cpp
        OnAccessStatusFile.h
        Logger.cpp
        Logger.h
        ProcessPriorityUtils.cpp
        ProcessPriorityUtils.h
        SoapdBootstrap.cpp
        SoapdBootstrap.h
        SoapdResources.cpp
        SoapdResources.h)

target_include_directories(soapdbootstrap
        PUBLIC
        ${CMAKE_SOURCE_DIR}/products/distribution/include
        ${JSON_SINGLE_INPUT}
)

target_link_libraries(soapdbootstrap PUBLIC
        common
        fanotifyhandler
        mountmonitor
        unixsocket
        )

install(TARGETS soapdbootstrap
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(soapdbootstrap
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )
