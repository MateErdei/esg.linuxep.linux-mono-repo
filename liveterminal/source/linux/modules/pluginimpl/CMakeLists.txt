configure_file(config.h.in config.h )

sophos_add_library(pluginimpl SHARED
        ApplicationPaths.cpp
        ApplicationPaths.h
        PluginCallback.cpp
        PluginCallback.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        PluginUtils.cpp
        PluginUtils.h
        Telemetry/Telemetry.cpp
        Telemetry/Telemetry.h
        Telemetry/TelemetryConsts.h
        config.h
        EXTRA_PROJECTS sessionrunnerimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
)
