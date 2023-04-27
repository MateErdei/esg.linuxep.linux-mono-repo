add_executable(${RA_PLUGIN_NAME}
        main.cpp)

include_directories(${RA_PLUGIN_NAME} PUBLIC ${CMAKE_BINARY_DIR}/modules/ResponseActions ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(${RA_PLUGIN_NAME} PUBLIC responseplugin)

SET_TARGET_PROPERTIES( ${RA_PLUGIN_NAME}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS ${RA_PLUGIN_NAME}
        DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

install(TARGETS
        ProxyUtilsLib
        obfuscationimpl
        HttpRequestsImpl
        curlwrapper
        pluginapi
        sslimpl
        ziputilities
        utilityimpl
        filesystemimpl
        logging
        applicationconfigurationimpl
        LIBRARY DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

