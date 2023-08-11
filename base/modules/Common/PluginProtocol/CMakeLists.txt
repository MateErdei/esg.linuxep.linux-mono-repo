# Generate proto files
INCLUDE_DIRECTORIES(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH})
#set(Protobuf_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc.sh)

if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
    message(STATUS "Found PROTOBUF Compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
else ()
    message(FATAL_ERROR "Could not find PROTOBUF Compiler")
endif ()

PROTOBUF_GENERATE_CPP(PLUGIN_API_MESSAGE_PROTO_SRC PLUGIN_API_MESSAGE_PROTO_H PluginAPIMessage.proto)
PROTOBUF_GENERATE_PYTHON(PLUGIN_API_MESSAGE_PROTO_PYTHON PluginAPIMessage.proto)

add_custom_target(generatePluginApiMessageProto
        ${CMAKE_COMMAND} -E copy_if_different
        ${PLUGIN_API_MESSAGE_PROTO_PYTHON} ${CMAKE_CURRENT_BINARY_DIR}/
        DEPENDS ${PLUGIN_API_MESSAGE_PROTO_PYTHON})


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
target_include_directories(pluginprotocolobject PUBLIC ${CMAKE_BINARY_DIR}/modules ../ZeroMQWrapper ../Reactor ../../ ${LOG4CPLUS_INCLUDE_DIR})
add_library(pluginprotocol SHARED $<TARGET_OBJECTS:pluginprotocolobject>)
add_dependencies(pluginprotocolobject generatePluginApiMessageProto)

target_link_libraries(pluginprotocol PUBLIC zeromqwrapperimpl reactorimpl ${LOG4CPLUS_LIBRARY} ${Protobuf_LIBRARIES})

SET_TARGET_PROPERTIES(pluginprotocol
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginprotocol
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)