add_library(pluginapiimpl SHARED
        ../PluginApi/DataMessage.h
        ../PluginApi/IListenerServer.h
        ../PluginApi/IProtocolSerializerFactory.h
        ../PluginApi/IProtocolSerializer.h
        ../PluginApi/IPluginApi.h
        ../PluginApi/ISensorDataCallback.h
        Protocol.cpp
        Protocol.h
        DataMessage.cpp
        AbstractListenerServer.cpp
        AbstractListenerServer.h
        ProtocolSerializerFactory.cpp
        ProtocolSerializerFactory.h
        ProtocolSerializer.cpp
        ProtocolSerializer.h MessageBuilder.h ../PluginApi/ISensorDataPublisher.h ../PluginApi/ISensorDataSubscriber.h MessageBuilder.cpp PluginApiImpl.cpp PluginApiImpl.h PluginCallBackHandler.cpp PluginCallBackHandler.h SensorDataPublisher.cpp SensorDataPublisher.h SensorDataSubscriber.cpp SensorDataSubscriber.h SharedSocketContext.cpp SharedSocketContext.h ../PluginApi/IPluginResourceManagement.h PluginResourceManagement.cpp PluginResourceManagement.h)

target_include_directories(pluginapiimpl PUBLIC ../PluginApi ../../Common )
target_link_libraries(pluginapiimpl PUBLIC threads reactorimpl )