

configure_file(${CMAKE_SOURCE_DIR}/installer/install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(${CMAKE_SOURCE_DIR}/installer/plugin.json.in ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json @ONLY )

install(FILES  ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json DESTINATION files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/installer/uninstall.sh
        DESTINATION files/plugins/${PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
