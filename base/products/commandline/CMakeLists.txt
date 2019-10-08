configure_file(registerCentral.in.sh registerCentral.sh @ONLY)
configure_file(pyCoverage.in.sh pyCoverage.sh @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyCoverage.sh
        DESTINATION files
        RENAME pyCoverage
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/registerCentral.sh
        DESTINATION files/base/bin
        RENAME registerCentral
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
