add_library(wdctlimpl STATIC
        wdctl_main.cpp
        wdctl_main.h
        wdctl_bootstrap.cpp
        wdctl_bootstrap.h
        StringVector.h
        Logger.h
        Logger.cpp)

target_include_directories(wdctlimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(wdctlimpl PUBLIC
        applicationconfigurationimpl
        filesystemimpl
        wdctlactions
        wdctlarguments
        logging)
