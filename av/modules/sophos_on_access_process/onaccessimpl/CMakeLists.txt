
add_library(onaccessimpl SHARED
        ClientSocketWrapper.cpp
        ClientSocketWrapper.h
        Logger.cpp
        Logger.h
        ../ScanRequestQueue/OnAccessScanRequest.cpp
        ../ScanRequestQueue/OnAccessScanRequest.h
        ../OnAccessTelemetryUtility/Logger.cpp
        ../OnAccessTelemetryUtility/Logger.h
        ../OnAccessTelemetryUtility/OnAccessTelemetryUtility.cpp
        ../OnAccessTelemetryUtility/OnAccessTelemetryUtility.h
        ScanRequestHandler.cpp
        ScanRequestHandler.h
        ../ScanRequestQueue/Logger.cpp
        ../ScanRequestQueue/Logger.h
        ../ScanRequestQueue/ScanRequestQueue.cpp
        ../ScanRequestQueue/ScanRequestQueue.h
)

target_link_libraries(onaccessimpl PUBLIC
        common
        scanmessages
        )

target_include_directories(onaccessimpl PUBLIC
        ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}/modules
        )

install(TARGETS onaccessimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(onaccessimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )
