
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mcsrouter.sh
        DESTINATION files/base/bin
        RENAME mcsrouter
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
