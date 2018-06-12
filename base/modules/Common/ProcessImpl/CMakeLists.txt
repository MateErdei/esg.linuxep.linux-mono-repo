
add_library(processimpl SHARED
        ../Process/IProcess.h
        ProcessImpl.cpp
        ProcessImpl.h)
target_include_directories(processimpl PUBLIC ../Process ../../Common)