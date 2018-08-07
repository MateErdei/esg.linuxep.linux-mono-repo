add_library(wdctlimpl STATIC
        wdctl_main.cpp
        wdctl_main.h
        wdctl_bootstrap.cpp
        wdctl_bootstrap.h
        ../wdctlarguments/Arguments.cpp
        ../wdctlarguments/Arguments.h
        StringVector.h
        Logger.h
        LoggingSetup.cpp
        LoggingSetup.h
        )

set( LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include )

find_library(LOG4CPLUS_LIBRARY
        NAMES log4cplus
        PATHS ${LOG4CPLUS_INPUT}/lib
        )

target_include_directories(wdctlimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(wdctlimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        applicationconfigurationimpl
        filesystemimpl
        wdctlactions)
