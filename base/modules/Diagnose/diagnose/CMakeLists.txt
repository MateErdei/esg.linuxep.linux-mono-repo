add_library(diagnoseimpl STATIC
            diagnose_main.cpp
            diagnose_main.h
            GatherFiles.cpp
            GatherFiles.h
            SystemCommands.cpp
            SystemCommands.h)

target_include_directories(diagnoseimpl PUBLIC filesystemimpl)

