
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


add_executable(sync_versioned_files
        sync_versioned_files/sync_versioned_files.cpp
        sync_versioned_files/sync_versioned_files.h
        sync_versioned_files/sync_versioned_files_main.cpp
        )
SET_TARGET_PROPERTIES(sync_versioned_files
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/modules)
target_link_libraries(sync_versioned_files PUBLIC
        ${STD_FILESYSTEM_IF_REQUIRED})
install(TARGETS sync_versioned_files
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin/)
