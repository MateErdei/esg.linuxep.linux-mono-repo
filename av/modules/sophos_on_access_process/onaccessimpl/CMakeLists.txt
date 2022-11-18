
add_library(onaccessimpl SHARED
        ClientSocketWrapper.cpp
        ClientSocketWrapper.h
        Logger.cpp
        Logger.h
        OnAccessTelemetryUtility.cpp
        OnAccessTelemetryUtility.h
        ScanRequestHandler.cpp
        ScanRequestHandler.h
        ScanRequestQueue.cpp
        ScanRequestQueue.h)

target_link_libraries(onaccessimpl  PUBLIC
        unixsocket
        )

install(TARGETS onaccessimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(onaccessimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
