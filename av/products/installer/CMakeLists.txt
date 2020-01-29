
configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json @ONLY )
configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )

install(FILES  ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json DESTINATION files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
