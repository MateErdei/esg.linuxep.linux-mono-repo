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
        ApplicationPaths.h
        ApplicationPaths.cpp
        OsqueryProcessImpl.h
        OsqueryProcessImpl.cpp
        IOsqueryProcess.h
        TelemetryConsts.h
        EXTRA_PROJECTS livequeryimpl procimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules  ${BOOST_INCLUDE_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${expatlib} ${zlib}
)
