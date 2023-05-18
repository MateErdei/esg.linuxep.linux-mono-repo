configure_file(config.h.in config.h )

sophos_add_library(pluginimpl SHARED
        PluginCallback.cpp
        PluginCallback.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        TaskQueue.cpp
        TaskQueue.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        DiskManager.cpp
        DiskManager.h
        Telemetry.cpp
        Telemetry.h
        config.h
        EXTRA_PROJECTS subscriberlib
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules ${XZUTILS_INCLUDE_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${LZMA_LIBRARY}
)

SET_TARGET_PROPERTIES(pluginimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
