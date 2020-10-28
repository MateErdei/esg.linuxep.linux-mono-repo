add_executable(sophos_extension main.cpp)

target_link_libraries(sophos_extension PUBLIC serverinfotable)

SET_TARGET_PROPERTIES(sophos_extension
    PROPERTIES 
    INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
    )

install(TARGETS sophos_extension
        DESTINATION files/plugins/${PLUGIN_NAME}/extensions
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE  GROUP_READ GROUP_EXECUTE)


