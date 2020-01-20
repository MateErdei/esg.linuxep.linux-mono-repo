configure_file(config.h.in config.h )

sophos_add_library(pluginimpl SHARED
        PluginCallback.cpp
        PluginCallback.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        StringReplace.cpp
        StringReplace.h
        Telemetry.cpp
        Telemetry.h
        config.h
        ../manager/scanprocessmonitor/ScanProcessMonitor.cpp
        ../manager/scanprocessmonitor/ScanProcessMonitor.h
        ../manager/scanprocessmonitor/Logger.cpp
        ../manager/scanprocessmonitor/Logger.h
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
)
