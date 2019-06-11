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
            $<TARGET_OBJECTS:filesystemimplobject>
            $<TARGET_OBJECTS:applicationconfigurationimplobject>
            ../../Common/UtilityImpl/TimeUtils.cpp
            ../../Common/UtilityImpl/TimeUtils.h
            ../../Common/UtilityImpl/StringUtils.h
        )
