include(proto/CMakeLists.txt)

add_library(pluginprotocolobject OBJECT
        DataMessage.h
        DataMessage.cpp
        AbstractListenerServer.cpp
        AbstractListenerServer.h
        MessageBuilder.cpp
        MessageBuilder.h
        ${PLUGIN_API_MESSAGE_PROTO_SRC}
        ${PLUGIN_API_MESSAGE_PROTO_H}
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
target_include_directories(pluginprotocolobject PUBLIC ../ZeroMQWrapper ../Reactor ../../ ${LOG4CPLUS_INCLUDE_DIR})
add_library(pluginprotocol SHARED $<TARGET_OBJECTS:pluginprotocolobject>)
add_dependencies(pluginprotocolobject generatePluginApiMessageProto)

target_link_libraries(pluginprotocol PUBLIC zeromqwrapperimpl reactorimpl ${LOG4CPLUS_LIBRARY} ${Protobuf_LIBRARIES})

SET_TARGET_PROPERTIES(pluginprotocol
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginprotocol
        LIBRARY DESTINATION files/base/lib64)