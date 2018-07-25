add_library(watchdogimpl STATIC
        watchdog_main.cpp
        watchdog_main.h
        Logger.h
        PluginProxy.cpp PluginProxy.h)

target_link_libraries(watchdogimpl PUBLIC pluginregistryimpl threads processimpl)