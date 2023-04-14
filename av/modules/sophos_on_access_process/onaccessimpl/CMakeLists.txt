
add_library(onaccessimpl SHARED
        ClientSocketWrapper.cpp
        ClientSocketWrapper.h
        Logger.cpp
        Logger.h
        OnAccessScanRequest.cpp
        OnAccessScanRequest.h
        OnAccessTelemetryUtility.cpp
        OnAccessTelemetryUtility.h
        ScanRequestHandler.cpp
        ScanRequestHandler.h
        ScanRequestQueue.cpp
        ScanRequestQueue.h)

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
