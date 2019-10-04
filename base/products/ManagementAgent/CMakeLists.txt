add_executable(sophos_managementagent
        main.cpp)
add_dependencies(sophos_managementagent managementagentimpl)
target_link_libraries(sophos_managementagent
        managementagentimpl
        pluginregistryimpl
        pluginprotocol
        )

SET_TARGET_PROPERTIES(sophos_managementagent
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS sophos_managementagent
        DESTINATION files/base/bin)

configure_file(managementagent.json.in  managementagent.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/managementagent.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ)