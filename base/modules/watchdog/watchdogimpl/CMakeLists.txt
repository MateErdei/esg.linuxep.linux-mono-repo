add_library(watchdogservice SHARED
        WatchdogServiceLine.cpp
        WatchdogServiceLine.h
        IWatchdogRequest.h
        Logger.h
        Logger.cpp
        WatchdogServiceException.h)

add_library(watchdogimpl STATIC
        watchdog_main.cpp
        watchdog_main.h
        PluginProxy.cpp
        PluginProxy.h
        Watchdog.cpp
        Watchdog.h
        )

target_link_libraries(watchdogimpl PUBLIC
        pluginregistryimpl
        watchdogservice
        ${LOG4CPLUS_LIBRARY}
        )
target_include_directories(watchdogservice SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(watchdogservice PUBLIC
        logging
        processmonitoringimpl
        pluginapiimpl
        plugincommunicationimplcommon
        telemetryhelperimpl
        commscomponentimpl
        )

SET_TARGET_PROPERTIES(watchdogservice
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS watchdogservice
        LIBRARY DESTINATION files/base/lib64)
