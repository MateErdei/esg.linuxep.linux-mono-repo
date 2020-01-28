
add_executable(avscanner
        cmd_scanner.cpp
        )

target_include_directories(avscanner PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(avscanner PRIVATE
        filewalker
        unixsocket
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )

SET_TARGET_PROPERTIES( avscanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS avscanner
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE
        )


## Create a duplicate for scheduled scans - (Add capability to read all files at install time)
ADD_CUSTOM_COMMAND(
        TARGET avscanner
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_NAME:avscanner>
                ${CMAKE_BINARY_DIR}/scheduled_file_walker
)

INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/scheduled_file_walker
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)
