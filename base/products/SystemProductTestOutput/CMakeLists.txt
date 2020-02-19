add_subdirectory(ZMQChecker)
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
        ${CMAKE_BINARY_DIR}/tests/manualTools/FaultInjectionLogger
        ${CMAKE_BINARY_DIR}/tests/manualTools/PickYourPoison
        ${CMAKE_SOURCE_DIR}/redist/openssl/lib64/libcrypto.so.1.1
        ${CMAKE_SOURCE_DIR}/redist/openssl/lib64/libssl.so.1.1
        ${CMAKE_SOURCE_DIR}/redist/gcc/lib64/libssp.so.0.0.0
        ${CMAKE_SOURCE_DIR}/redist/openssl/64/bin/openssl
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/testUtils
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)