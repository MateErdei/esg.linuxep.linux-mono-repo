
add_library(processimplobject OBJECT
        ProcessImpl.cpp
        ProcessImpl.h
        ArgcAndEnv.h
        StdPipeThread.cpp
        StdPipeThread.h
        PipeHolder.cpp
        PipeHolder.h
        Logger.h
        Logger.cpp
        ProcessInfo.cpp
        ProcessInfo.h
        IProcessHolder.h
        BoostProcessHolder.cpp
        BoostProcessHolder.h)
target_include_directories(processimplobject PUBLIC ../Process ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
add_library(processimpl SHARED $<TARGET_OBJECTS:processimplobject> )

target_include_directories(processimpl PUBLIC ../Process ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(processimpl PUBLIC ${LOG4CPLUS_LIBRARY} logging threads)

SET_TARGET_PROPERTIES( processimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS processimpl
        LIBRARY DESTINATION files/base/lib64)
