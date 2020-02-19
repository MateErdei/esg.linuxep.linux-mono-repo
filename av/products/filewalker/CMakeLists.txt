
add_executable(avscanner
        cmd_scanner.cpp
        )

target_include_directories(avscanner PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(avscanner PRIVATE
        avscannerimpl
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

add_executable(scheduled_file_walker_launcher
        scheduled_file_walker_launcher.c
        ../capability/PassOnCapability.c)

target_link_libraries(scheduled_file_walker_launcher  PUBLIC cap)

install(TARGETS scheduled_file_walker_launcher
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
