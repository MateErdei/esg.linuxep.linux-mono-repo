
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