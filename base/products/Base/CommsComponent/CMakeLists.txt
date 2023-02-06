add_executable(CommsComponent
        main.cpp)

add_dependencies(CommsComponent commscomponentimpl)

target_link_libraries(CommsComponent
        commscomponentimpl
        )

SET_TARGET_PROPERTIES(CommsComponent
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS CommsComponent
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)

configure_file(commscomponent.json.in  commscomponent.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/commscomponent.json
        DESTINATION ${BASE_DIST_PREFIX}/installer/plugins
        PERMISSIONS OWNER_READ)