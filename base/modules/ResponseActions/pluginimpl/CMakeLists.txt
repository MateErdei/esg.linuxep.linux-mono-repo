configure_file(config.h.in config.h )

add_library(rapluginimpl SHARED
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

include_directories(rapluginimpl BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(rapluginimpl PUBLIC
        logging
        pluginapiimpl
        applicationconfigurationimpl
        telemetryhelperimpl
        filesystemimplobject)

SET_TARGET_PROPERTIES(rapluginimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS rapluginimpl
        LIBRARY DESTINATION ra/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(rapluginimpl copy_libs)