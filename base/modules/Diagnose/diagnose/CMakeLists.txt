add_library(diagnoseimpl STATIC
            diagnose_main.cpp
            diagnose_main.h
            GatherFiles.cpp
            GatherFiles.h
            SystemCommands.cpp
            SystemCommands.h
            Strings.h
            $<TARGET_OBJECTS:filesystemimplobject>
            ../../Common/UtilityImpl/TimeUtils.cpp
            ../../Common/UtilityImpl/TimeUtils.h)
