add_library(pluginapiimpl SHARED
        ../PluginProtocol/IProtocolSerializer.h
        ../PluginApi/IPluginApi.h
        ../PluginApi/ISensorDataCallback.h
        ../PluginProtocol/Protocol.cpp
        ../PluginProtocol/Protocol.h
        ../PluginProtocol/AbstractListenerServer.cpp
        ../PluginProtocol/AbstractListenerServer.h
        ../PluginProtocol/ProtocolSerializerFactory.cpp
        ../PluginProtocol/ProtocolSerializerFactory.h
        ../PluginProtocol/ProtocolSerializer.cpp
        ../PluginProtocol/ProtocolSerializer.h ../PluginProtocol/MessageBuilder.h
        ../PluginApi/ISensorDataPublisher.h
        ../PluginApi/ISensorDataSubscriber.h
        ../PluginProtocol/MessageBuilder.cpp
        PluginApiImpl.cpp
        PluginApiImpl.h
        PluginCallBackHandler.cpp
        PluginCallBackHandler.h
        SensorDataPublisher.cpp
        SensorDataPublisher.h
        SensorDataSubscriber.cpp
        SensorDataSubscriber.h
        SharedSocketContext.cpp
        SharedSocketContext.h
        ../PluginApi/IPluginResourceManagement.h
        PluginResourceManagement.cpp
        PluginResourceManagement.h
        ../PluginApi/IPluginCallback.h)

target_include_directories(pluginapiimpl PUBLIC ../PluginApi ../../Common )
target_link_libraries(pluginapiimpl PUBLIC threads reactorimpl pluginprotocol )