
add_library(processimpl SHARED
        ../Process/EnvPair.h
        ../Process/IProcess.h
        ProcessImpl.cpp
        ProcessImpl.h
        ArgcAndEnv.h
        StdPipeThread.cpp
        StdPipeThread.h
        PipeHolder.cpp
        PipeHolder.h
        Logger.h)
target_include_directories(processimpl PUBLIC ../Process ../../Common)

SET_TARGET_PROPERTIES( processimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS processimpl
        LIBRARY DESTINATION files/base/lib64)
