
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/registerCentral.sh
        DESTINATION files/base/bin
        RENAME registerCentral
        PERMISSIONS OWNER_READ OWNER_EXECUTE)
