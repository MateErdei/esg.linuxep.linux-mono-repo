add_executable(${plugin_name}
        main.cpp
        )
target_link_libraries(${plugin_name} PUBLIC pluginimpl eventqueuelib eventwriterlib eventjournal heartbeat)

SET_TARGET_PROPERTIES( ${plugin_name}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS ${plugin_name}
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)
