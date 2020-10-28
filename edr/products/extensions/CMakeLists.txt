add_executable(sophos_serverinfo main.cpp)

target_link_libraries(sophos_serverinfo PUBLIC serverinfotable)

SET_TARGET_PROPERTIES(sophos_serverinfo
    PROPERTIES 
    INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
    )

install(TARGETS sophos_serverinfo
        DESTINATION files/plugins/${PLUGIN_NAME}/extensions
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE  GROUP_READ GROUP_EXECUTE)


