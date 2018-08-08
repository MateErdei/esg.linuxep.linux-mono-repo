
add_library(updateschedulerimpl main.cpp
        PluginCallback.cpp
        PluginCallback.h
        QueueTask.cpp
        QueueTask.h
        Logger.h
        LoggingSetup.cpp
        LoggingSetup.h
        PluginAdapter.cpp
        PluginAdapter.h
        Telemetry.cpp
        Telemetry.h
        UpdateScheduler.cpp
        UpdateScheduler.h)

set( LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include )

find_library(LOG4CPLUS_LIBRARY
        NAMES log4cplus
        PATHS ${LOG4CPLUS_INPUT}/lib
        )
target_include_directories(updateschedulerimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(updateschedulerimpl pluginapiimpl ${LOG4CPLUS_LIBRARY})