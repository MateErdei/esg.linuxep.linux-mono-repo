
add_library(processimpl SHARED
        ../Process/IProcess.h
        ProcessImpl.cpp
        ProcessImpl.h ArgcAndEnv.h)
target_include_directories(processimpl PUBLIC ../Process ../../Common)


install(TARGETS processimpl
        LIBRARY DESTINATION files/base/lib64)
