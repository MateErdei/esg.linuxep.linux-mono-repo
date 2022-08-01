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
        ${INPUT}/openssl/lib64/libcrypto.so.1.1
        ${INPUT}/openssl/lib64/libssl.so.1.1
        ${INPUT}/gcc/lib64/libssp.so.0.0.0
        ${INPUT}/openssl/64/bin/openssl
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/testUtils
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)
