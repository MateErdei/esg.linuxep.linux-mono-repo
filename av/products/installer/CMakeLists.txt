configure_file(install.sh.in ${PROJECT_BINARY_DIR}/install.sh @ONLY )
configure_file(plugin.json.in ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json @ONLY )
configure_file(on_access_process.json.in ${PROJECT_BINARY_DIR}/on_access_process.json @ONLY )
configure_file(threat_detector.json.in ${PROJECT_BINARY_DIR}/threat_detector.json @ONLY )
configure_file(uninstall.sh.in ${PROJECT_BINARY_DIR}/uninstall.sh @ONLY )

install(FILES  ${PROJECT_BINARY_DIR}/${PLUGIN_NAME}.json DESTINATION files/base/pluginRegistry)
install(FILES  ${PROJECT_BINARY_DIR}/on_access_process.json DESTINATION files/base/pluginRegistry)
install(FILES  ${PROJECT_BINARY_DIR}/threat_detector.json DESTINATION files/base/pluginRegistry)

install(FILES ${PROJECT_BINARY_DIR}/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${PROJECT_BINARY_DIR}/uninstall.sh
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS program_options)

add_executable(sync_versioned_files
        sync_versioned_files/sync_versioned_files.cpp
        sync_versioned_files/sync_versioned_files.h
        sync_versioned_files/sync_versioned_files_main.cpp
        )
SET_TARGET_PROPERTIES(sync_versioned_files
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")
target_include_directories(sync_versioned_files PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${BOOST_INCLUDEDIR})

target_link_libraries(sync_versioned_files PUBLIC
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${Boost_LIBRARIES}
        datatypes
        )
install(TARGETS sync_versioned_files
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin/)
