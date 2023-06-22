add_library(wdctlactions STATIC
        ../wdctlarguments/Arguments.h
        Logger.h
        Logger.cpp
        CopyPlugin.cpp
        CopyPlugin.h
        StartAction.cpp
        StartAction.h
        StopAction.cpp
        StopAction.h
        Action.cpp
        Action.h
        ZMQAction.cpp
        ZMQAction.h
        RemoveAction.cpp
        RemoveAction.h
        IsRunning.cpp
        IsRunning.h)

target_include_directories(wdctlactions SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(wdctlactions PUBLIC
        ${LOG4CPLUS_LIBRARY}
        applicationconfigurationimpl
        filesystemimpl
        zmqwrapperapiimpl
        wdctlarguments)
