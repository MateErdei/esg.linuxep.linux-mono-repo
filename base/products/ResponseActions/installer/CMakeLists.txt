
configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(cleanuprealm.dat.in ${PROJECT_BINARY_DIR}/cleanuprealm.dat @ONLY )
configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${ra_plugin_name}.json @ONLY )
configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )

install(FILES  ${PROJECT_BINARY_DIR}/${ra_plugin_name}.json DESTINATION files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION ra
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/cleanupinstall.sh
        DESTINATION ra
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/cleanuprealm.dat
        DESTINATION ra
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION ra/files/plugins/${ra_plugin_name}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

