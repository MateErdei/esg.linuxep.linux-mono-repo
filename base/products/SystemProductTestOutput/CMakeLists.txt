add_subdirectory(ZMQChecker)
set( PLUGIN_PROTOCOL_OUTPUT_FOLDER  ${CMAKE_BINARY_DIR}/modules/Common/PluginProtocol/ )

install(FILES
        ${PLUGIN_PROTOCOL_OUTPUT_FOLDER}/PluginAPIMessage_pb2.py
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/extra_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/nova_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/qa_mcs_rootca.crt
        ${CMAKE_BINARY_DIR}/tests/manualTools/FaultInjectionLogger
        ${CMAKE_BINARY_DIR}/tests/manualTools/PickYourPoison
        ${CMAKE_BINARY_DIR}/tests/manualTools/ignoreSignals
        ${CMAKE_BINARY_DIR}/tests/manualTools/JailBreakerTest
        ${CMAKE_BINARY_DIR}/tests/manualTools/UseCommsComponent
        ${CMAKE_BINARY_DIR}/tests/manualTools/RunHttpRequest
        ${CMAKE_BINARY_DIR}/tests/manualTools/EventPubSub
#        ${CMAKE_BINARY_DIR}/tests/Common/HttpRequesterImpl/HttpRequesterLiveNetworkTests
        ${CMAKE_SOURCE_DIR}/tests/Common/HttpRequesterImpl/HttpTestServer.py
        /build/redist/openssl/lib64/libcrypto.so.1.1
        /build/redist/openssl/lib64/libssl.so.1.1
        /build/redist/gcc/lib64/libssp.so.0.0.0
        /build/redist/openssl/64/bin/openssl
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/testUtils
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)
