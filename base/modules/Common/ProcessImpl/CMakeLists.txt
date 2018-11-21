
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
        Logger.h Logger.cpp)
target_include_directories(processimpl PUBLIC ../Process ../../Common ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(processimpl PUBLIC ${LOG4CPLUS_LIBRARY} )

SET_TARGET_PROPERTIES( processimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS processimpl
        LIBRARY DESTINATION files/base/lib64)
