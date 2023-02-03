configure_file(registerCentral.in.sh registerCentral.sh @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/registerCentral.sh
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin
        RENAME registerCentral
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
