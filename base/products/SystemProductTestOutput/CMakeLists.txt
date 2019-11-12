set( PLUGIN_PROTOCOL_OUTPUT_FOLDER  ${CMAKE_BINARY_DIR}/modules/Common/PluginProtocol/ )

install(FILES
        ${PLUGIN_PROTOCOL_OUTPUT_FOLDER}/PluginAPIMessage_pb2.py
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/Common.capnp
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/Credentials.capnp
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/Process.capnp
        ${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/PortScanning.capnp
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/extra_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/nova_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/qa_mcs_rootca.crt
        ${CMAKE_BINARY_DIR}/tests/manualTools/LoggerLimit
        ${CMAKE_BINARY_DIR}/tests/manualTools/FaultInjectionLogger
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
