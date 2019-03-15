add_library(diagnoseimpl STATIC
            diagnose_main.cpp
            diagnose_main.h
            GatherFiles.cpp
            GatherFiles.h)

target_include_directories(diagnoseimpl PUBLIC filesystemimpl)

