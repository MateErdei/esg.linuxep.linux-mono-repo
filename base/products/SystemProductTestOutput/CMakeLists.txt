set( PLUGIN_PROTOCOL_OUTPUT_FOLDER  ${CMAKE_BINARY_DIR}/modules/Common/PluginProtocol/ )

install(FILES
        ${PLUGIN_PROTOCOL_OUTPUT_FOLDER}/PluginAPIMessage_pb2.py
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/Common.capnp
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/Credentials.capnp
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/PortScanning.capnp
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
