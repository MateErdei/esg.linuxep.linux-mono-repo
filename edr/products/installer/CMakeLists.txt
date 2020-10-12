
configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(cleanuprealm.dat.in ${PROJECT_BINARY_DIR}/cleanuprealm.dat @ONLY )
configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json @ONLY )
configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )
configure_file(syslog_configs/rsyslog_sophos-spl.conf ${PROJECT_BINARY_DIR}/syslog_configs/rsyslog_sophos-spl.conf @ONLY )

install(FILES ${OSQUERY_BINARY}
        DESTINATION
        files/plugins/${PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(DIRECTORY ${OSQUERY_LENSES_DIR}
        DESTINATION files/plugins/${PLUGIN_NAME}/osquery/
        FILES_MATCHING PATTERN *.*
        )

install(FILES  ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json DESTINATION files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )
install(FILES ${DISTRIBUTION_TOOL_DIR}/cleanupinstall.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/cleanuprealm.dat
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION files/plugins/${PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/plugins/${PLUGIN_NAME}
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libasan.so.*
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/plugins/${PLUGIN_NAME}
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libubsan.so.*
        )

install(FILES  ${PROJECT_BINARY_DIR}/syslog_configs/rsyslog_sophos-spl.conf DESTINATION files/plugins/${PLUGIN_NAME}/etc/syslog_configs)
