add_executable(sophos_actions main.cpp)

target_link_libraries(sophos_actions PUBLIC responseactionmain)

SET_TARGET_PROPERTIES(sophos_actions
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS sophos_actions
        DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE GROUP_READ GROUP_EXECUTE)
