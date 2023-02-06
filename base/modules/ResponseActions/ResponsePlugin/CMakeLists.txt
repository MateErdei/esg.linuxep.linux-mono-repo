configure_file(config.h.in config.h )

add_library(responseplugin SHARED
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
        Telemetry.cpp
        Telemetry.h)

include_directories(responseplugin BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(responseplugin PUBLIC
        logging
        pluginapiimpl
        applicationconfigurationimpl
        telemetryhelperimpl
        filesystemimplobject)

SET_TARGET_PROPERTIES(responseplugin
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS responseplugin
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(responseplugin copy_libs)