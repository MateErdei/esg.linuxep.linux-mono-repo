configure_file(mcsrouter.in.sh mcsrouter.sh @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mcsrouter.sh
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin
        RENAME mcsrouter
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
configure_file(mcsrouter.json.in  mcsrouter.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mcsrouter.json
        DESTINATION ${BASE_DIST_PREFIX}/installer/plugins
        PERMISSIONS OWNER_READ)
