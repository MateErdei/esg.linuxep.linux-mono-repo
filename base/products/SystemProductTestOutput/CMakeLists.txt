add_subdirectory(ZMQChecker)
set( PLUGIN_PROTOCOL_OUTPUT_FOLDER  ${CMAKE_BINARY_DIR}/modules/Common/PluginProtocol/ )

install(FILES
        ${PLUGIN_PROTOCOL_OUTPUT_FOLDER}/PluginAPIMessage_pb2.py
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/extra_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/nova_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/modules/mcsrouter/engine/qa_mcs_rootca.crt
        ${CMAKE_SOURCE_DIR}/tests/Common/HttpRequesterImpl/HttpTestServer.py
        ${CMAKE_SOURCE_DIR}/products/distribution/generateSDDSImport.py
        ${CMAKE_SOURCE_DIR}/products/distribution/generateSDDSImportAndManifestDat.py
        ${CMAKE_SOURCE_DIR}/products/distribution/generateManifestDat.py
        ${CMAKE_SOURCE_DIR}/products/distribution/readVersion.py
        ${CMAKE_SOURCE_DIR}/products/distribution/fileInfo.py
        ${OPENSSL_INPUT}/bin/openssl
        ${OPENSSL_INPUT}/lib64/libcrypto.so.3
        ${OPENSSL_INPUT}/lib64/libssl.so.3
        ${INPUT}/gcc/lib64/libssp.so.0.0.0
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/testUtils
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/../common/TA/libs
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/../common/TA/robot
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)