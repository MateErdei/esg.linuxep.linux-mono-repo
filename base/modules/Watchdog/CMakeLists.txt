add_library(watchdogimpl SHARED
        Watchdog.cpp
        Watchdog.h
        Logger.h
        )

target_link_libraries(watchdogimpl PUBLIC pluginregistryimpl)

