configure_file(config.h.in config.h )

add_library(pluginimpl SHARED
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
        config.h)
target_include_directories(pluginimpl PUBLIC ${pluginapiinclude}  ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pluginimpl PUBLIC ${pluginapilib} ${zmqlib} ${log4cpluslib})
