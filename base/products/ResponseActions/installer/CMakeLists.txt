
configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(cleanuprealm.dat.in ${PROJECT_BINARY_DIR}/cleanuprealm.dat @ONLY )
configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${RA_PLUGIN_NAME}.json @ONLY )
configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )

install(FILES  ${PROJECT_BINARY_DIR}/${RA_PLUGIN_NAME}.json DESTINATION ${RA_DIST_PREFIX}/files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION ${RA_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/cleanupinstall.sh
        DESTINATION ${RA_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/checkAndRunExtraUpgrade.sh
        DESTINATION ${RA_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/cleanuprealm.dat
        DESTINATION ${RA_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

