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

target_link_libraries(pluginimpl PUBLIC
        logging
        pluginapiimpl
        applicationconfigurationimpl
        telemetryhelperimpl
        filesystemimplobject)

SET_TARGET_PROPERTIES(pluginimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

add_dependencies(pluginimpl copy_libs)