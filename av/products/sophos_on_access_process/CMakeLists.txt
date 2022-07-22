add_executable(soapd
        sophos_on_access_process_main.cpp)
target_link_libraries(soapd PUBLIC
        )

SET_TARGET_PROPERTIES( soapd
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS soapd
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE )
