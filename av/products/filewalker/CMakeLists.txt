
add_executable(cmd_scanner
        cmd_scanner.cpp
        )

target_include_directories(cmd_scanner PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(cmd_scanner PRIVATE
        filewalker
        unixsocket
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )



SET_TARGET_PROPERTIES( cmd_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS cmd_scanner
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

