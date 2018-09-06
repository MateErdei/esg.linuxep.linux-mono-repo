add_library(pluginprotocolobject OBJECT
        DataMessage.h
        DataMessage.cpp
        ../PluginApiImpl/Logger.h
        AbstractListenerServer.cpp
        AbstractListenerServer.h
        MessageBuilder.cpp
        MessageBuilder.h
        Protocol.h
        Protocol.cpp
        ProtocolSerializer.h
        ProtocolSerializer.cpp
        IProtocolSerializer.h
        ProtocolSerializerFactory.h
        ProtocolSerializerFactory.cpp
        Logger.h
        Logger.cpp
        )
target_include_directories(pluginprotocolobject PUBLIC ../ZeroMQWrapper ../Reactor ../../ ${LOG4CPLUS_INCLUDE_DIR} )
add_library(pluginprotocol SHARED $<TARGET_OBJECTS:pluginprotocolobject> )

target_link_libraries(pluginprotocol PUBLIC zeromqwrapperimpl reactorimpl ${LOG4CPLUS_LIBRARY})

SET_TARGET_PROPERTIES( pluginprotocol
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginprotocol
        LIBRARY DESTINATION files/base/lib64)