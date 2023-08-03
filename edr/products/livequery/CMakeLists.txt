add_executable(sophos_livequery main.cpp)

target_link_libraries(sophos_livequery PUBLIC livemain sqlite_library)

SET_TARGET_PROPERTIES(sophos_livequery
    PROPERTIES 
    INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
    )

install(TARGETS sophos_livequery 
        DESTINATION files/plugins/${PLUGIN_NAME}/bin/
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE  GROUP_READ GROUP_EXECUTE)
