configure_file(config.h.in config.h )

add_library(responseplugin SHARED
        ActionRunner.cpp
        ActionRunner.h
        ActionType.h
        ApplicationPaths.cpp
        ApplicationPaths.h
        ../RACommon/ResponseActionsCommon.h
        ../RACommon/ResponseActionsCommon.cpp
        IActionRunner.h
        Logger.cpp
        Logger.h
        PluginAdapter.cpp
        PluginAdapter.h
        PluginCallback.cpp
        PluginCallback.h
        PluginUtils.cpp
        PluginUtils.h
        TaskQueue.cpp
        TaskQueue.h
        Telemetry.cpp
        Telemetry.h
        ../RACommon/toUtf8Exception.cpp
        ../RACommon/toUtf8Exception.h)

include_directories(responseplugin BEFORE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/products/PluginApi/pluginapi/include
        ${BOOST_INCLUDE_DIR}
        )

target_link_libraries(responseplugin PUBLIC
        pluginapi
        ${Protobuf_LIBRARY}
        ${EXPAT_LIBRARY}
        ${ZeroMQ_LIBRARY}
        ${BOOST_LOCALE}
        )

SET_TARGET_PROPERTIES(responseplugin
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS responseplugin
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

add_dependencies(responseplugin copy_libs)