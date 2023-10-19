
configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(cleanuprealm.dat.in ${PROJECT_BINARY_DIR}/cleanuprealm.dat @ONLY )

if(MODE STREQUAL "COVERAGE")
    configure_file(plugin.coverage.json.in ${PROJECT_BINARY_DIR}/${LR_PLUGIN_NAME}.json @ONLY )
else()
    configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${LR_PLUGIN_NAME}.json @ONLY )
endif()

configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )

install(FILES ${LIVETERMINAL}
        DESTINATION
        files/plugins/${LR_PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES  ${PROJECT_BINARY_DIR}/${LR_PLUGIN_NAME}.json DESTINATION files/base/pluginRegistry)

install(FILES
        ${PROJECT_BINARY_DIR}/install.sh
        ${DISTRIBUTION_TOOL_DIR}/cleanupinstall.sh
        ${DISTRIBUTION_TOOL_DIR}/checkAndRunExtraUpgrade.sh
        ${PROJECT_BINARY_DIR}/cleanuprealm.dat
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )


install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION files/plugins/${LR_PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/plugins/${LR_PLUGIN_NAME}
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libasan.so.*
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/plugins/${LR_PLUGIN_NAME}
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libubsan.so.*
        )