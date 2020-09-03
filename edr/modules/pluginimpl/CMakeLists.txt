sophos_add_library(pluginimpl SHARED
        PluginCallback.cpp
        PluginCallback.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        Logger.cpp
        OsqueryDataManager.cpp
        OsqueryDataManager.h
        PluginAdapter.cpp
        PluginAdapter.h
        StringReplace.cpp
        StringReplace.h
        Telemetry.cpp
        Telemetry.h
        ApplicationPaths.h
        ApplicationPaths.cpp
        OsqueryProcessImpl.h
        OsqueryProcessImpl.cpp
        OsqueryLogger.h
        OsqueryLogger.cpp
        IOsqueryProcess.h
        TelemetryConsts.h
        OsqueryStarted.h
        OsqueryStarted.cpp
        OsqueryConfigurator.h
        OsqueryConfigurator.cpp
        SystemConfigurator.h
        SystemConfigurator.cpp
        OsqueryLogIngest.h
        OsqueryLogIngest.cpp
        EXTRA_PROJECTS queryrunnerimpl procimpl
        EXTRA_INCLUDES ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules ${BOOST_INCLUDE_DIR}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${expatlib} ${zlib}
)
