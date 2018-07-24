add_library(watchdogimpl STATIC
        watchdog_main.cpp
        watchdog_main.h
        Logger.h
        )

target_link_libraries(watchdogimpl PUBLIC pluginregistryimpl)