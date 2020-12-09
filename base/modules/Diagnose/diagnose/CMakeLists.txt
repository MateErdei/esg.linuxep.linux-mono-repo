add_library(diagnoseimpl STATIC
        CheckForTar.cpp
        CheckForTar.h
        diagnose_main.cpp
        diagnose_main.h
        GatherFiles.cpp
        GatherFiles.h
        SystemCommands.cpp
        SystemCommands.h
        Strings.h
        Logger.h
        Logger.cpp
        SystemCommandException.h
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:utilityimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:processimplobject>
        $<TARGET_OBJECTS:threadsobject>
        ../../Common/UtilityImpl/TimeUtils.cpp
        ../../Common/UtilityImpl/TimeUtils.h
        ../../Common/UtilityImpl/StringUtils.h
        )

target_include_directories(diagnoseimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(diagnoseimpl PUBLIC logging)
