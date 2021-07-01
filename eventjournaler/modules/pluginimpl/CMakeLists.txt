configure_file(config.h.in config.h )

sophos_add_library(pluginimpl SHARED
        PluginCallback.cpp
        PluginCallback.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        Telemetry.cpp
        Telemetry.h
        config.h
        EXTRA_PROJECTS subscriberlib
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
)
