
add_library(onAccessConfigMonitor SHARED
        OnAccessConfigMonitor.cpp
        OnAccessConfigMonitor.h
        Logger.cpp
        Logger.h
        )

target_link_libraries(onAccessConfigMonitor  PUBLIC
        unixsocket
        )

install(TARGETS onAccessConfigMonitor
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(onAccessConfigMonitor
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
