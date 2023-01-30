configure_file(config.h.in config.h )

add_library(pluginimpl SHARED
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
        Telemetry.h)

include_directories(pluginimpl BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(pluginimpl PUBLIC ${zmqlib} ${LOG4CPLUS_LIBRARY} ${protobuflib})