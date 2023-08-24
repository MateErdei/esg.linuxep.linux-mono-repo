add_library(watchdogimpl STATIC
        watchdog_main.cpp
        watchdog_main.h
        PluginProxy.cpp
        PluginProxy.h
        Watchdog.cpp
        Watchdog.h
        UserGroupUtils.cpp
        UserGroupUtils.h
        Logger.h
        Logger.cpp
        WatchdogServiceLine.cpp
        WatchdogServiceLine.h
        )

target_link_libraries(watchdogimpl PUBLIC
        pluginregistryimpl
        watchdogservice
        ${LOG4CPLUS_LIBRARY}
        )
