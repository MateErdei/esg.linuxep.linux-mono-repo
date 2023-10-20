add_executable(avscanner
        cmd_scanner.cpp
        )

target_include_directories(avscanner PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(avscanner PRIVATE
        avscannerimpl
        filewalker
        unixsocket
        ${STD_FILESYSTEM_IF_REQUIRED}
        )

SET_TARGET_PROPERTIES( avscanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS avscanner
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE
        )

add_executable(scheduled_file_walker_launcher
        scheduled_file_walker_launcher.c
        ../capability/PassOnCapability.c)

target_link_libraries(scheduled_file_walker_launcher  PUBLIC cap)
target_include_directories(scheduled_file_walker_launcher PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}
)

install(TARGETS scheduled_file_walker_launcher
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
