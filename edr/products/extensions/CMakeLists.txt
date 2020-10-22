add_executable(sophos_serverinfo.ext main.cpp)

target_link_libraries(sophos_serverinfo.ext PUBLIC livemain)

SET_TARGET_PROPERTIES(sophos_serverinfo.ext 
    PROPERTIES 
    INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
    )

install(TARGETS sophos_serverinfo.ext 
        DESTINATION files/plugins/${PLUGIN_NAME}/extensions
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE  GROUP_READ GROUP_EXECUTE)


