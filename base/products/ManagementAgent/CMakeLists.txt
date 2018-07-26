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

install(TARGETS sophos_managementagent
        DESTINATION files/base/bin)