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
        EXTRA_PROJECTS livequeryimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
)
