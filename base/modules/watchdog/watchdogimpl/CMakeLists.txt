add_library(watchdogimpl STATIC
        watchdog_main.cpp
        watchdog_main.h
        PluginProxy.cpp
        PluginProxy.h
        Watchdog.cpp
        Watchdog.h
        SignalHandler.cpp
        SignalHandler.h
        Logger.h
        Logger.cpp
        )

target_link_libraries(watchdogimpl PUBLIC
        pluginregistryimpl
        logging
        threads
        processimpl
        zeromqwrapperimpl
        ${LOG4CPLUS_LIBRARY}
        )

target_include_directories(watchdogimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
