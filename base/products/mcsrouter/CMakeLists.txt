
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mcsrouter.sh
        DESTINATION files/base/bin
        RENAME mcsrouter
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
configure_file(mcsrouter.json.in  mcsrouter.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mcsrouter.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ)
