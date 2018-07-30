add_executable(sophos_managementagent
        main.cpp)
add_dependencies(sophos_managementagent managementagentimpl)
target_link_libraries(sophos_managementagent
        managementagentimpl
        policyreceiverimpl
        directorywatcherimpl
        eventreceiverimpl
        statusreceiverimpl
        applicationconfigurationimpl
        mcsrouterplugincommunicationimpl
        taskqueueimpl
        pluginregistryimpl)

SET_TARGET_PROPERTIES(sophos_managementagent
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS sophos_managementagent
        DESTINATION files/base/bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/managementagent.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ)