
add_library(pluginapiobjects OBJECT
        ../PluginApi/IBaseServiceApi.h
        ../PluginApi/ISensorDataCallback.h
        ../PluginApi/ISensorDataPublisher.h
        ../PluginApi/ISensorDataSubscriber.h
        ../PluginApi/IPluginResourceManagement.h
        ../PluginApi/IPluginCallbackApi.h
        ../PluginApi/StatusInfo.h
        PluginApiImpl.cpp
        PluginApiImpl.h
        PluginCallBackHandler.cpp
        PluginCallBackHandler.h
        SensorDataPublisher.cpp
        SensorDataPublisher.h
        SensorDataSubscriber.cpp
        SensorDataSubscriber.h
        PluginResourceManagement.cpp
        PluginResourceManagement.h
        )

target_include_directories(pluginapiobjects PUBLIC ../PluginApi ../../Common )
add_library(pluginapiimpl SHARED $<TARGET_OBJECTS:pluginapiobjects> )

target_link_libraries(pluginapiimpl PUBLIC threads reactorimpl pluginprotocol )

SET_TARGET_PROPERTIES( pluginapiimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginapiimpl
        LIBRARY DESTINATION files/base/lib64)

add_library(pluginapi SHARED
        $<TARGET_OBJECTS:pluginapiobjects>
        $<TARGET_OBJECTS:pluginprotocolobject>
        $<TARGET_OBJECTS:zeromqwrapperimplobject>
        $<TARGET_OBJECTS:reactorimplobject>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        )

target_link_libraries(pluginapi PUBLIC ${ZeroMQ_LIBRARY} )
SET_TARGET_PROPERTIES( pluginapi
        PROPERTIES INSTALL_RPATH "$ORIGIN")

INCLUDE(FindLibraryFullPath)
FindLibraryFullPath( zmqfull zmq ${ZEROMQ_INPUT}/lib)

file(GLOB_RECURSE pluginapiheaders
        ../PluginApi/* )

add_custom_target(
        plugintar
        COMMAND mkdir -p pluginapi/include pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${pluginapiheaders} pluginapi/include
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pluginapi> pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${zmqfull} pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E tar czf pluginapi.tar.gz pluginapi
        COMMAND rm -rf pluginapi
)

add_dependencies(pluginapiimpl plugintar)
