
configure_file(config.h.in config.h )

include_directories(${plugin_name} ${pluginapiinclude}  ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_executable(${plugin_name}
        main.cpp
        FileSystem.cpp
        FileSystem.h
        PluginCallback.cpp
        PluginCallback.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        Logger.cpp
        PluginAdapter.cpp
        PluginAdapter.h
        StringReplace.cpp
        StringReplace.h
        Telemetry.cpp
        Telemetry.h)
target_link_libraries(${plugin_name} PUBLIC ${pluginapilib} ${zmqlib} ${log4cpluslib})


SET_TARGET_PROPERTIES( ${plugin_name}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${plugin_name}
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

