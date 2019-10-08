configure_file(registerCentral.in.sh registerCentral.sh @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/registerCentral.sh
        DESTINATION files/base/bin
        RENAME registerCentral
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
