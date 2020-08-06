add_executable(CommsComponent
        main.cpp)

add_dependencies(CommsComponent commscomponentimpl)

target_link_libraries(CommsComponent
        commscomponentimpl
        )

SET_TARGET_PROPERTIES(CommsComponent
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

# FIXME: LINUXDAR-2120 restore
#install(TARGETS CommsComponent
#        DESTINATION files/base/bin)
#
#configure_file(commscomponent.json.in  commscomponent.json @ONLY)

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/commscomponent.json
#        DESTINATION installer/plugins
#        PERMISSIONS OWNER_READ)