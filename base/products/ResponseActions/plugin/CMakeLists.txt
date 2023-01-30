add_executable(${ra_plugin_name}
        main.cpp)

message(${ra_plugin_name})
target_link_libraries(${ra_plugin_name} PUBLIC pluginimpl)

SET_TARGET_PROPERTIES( ${ra_plugin_name}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${ra_plugin_name}
        DESTINATION files/plugins/${ra_plugin_name}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

