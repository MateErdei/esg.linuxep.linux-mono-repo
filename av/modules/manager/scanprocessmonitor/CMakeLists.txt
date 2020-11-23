sophos_add_library(scanprocessmonitor SHARED
        ConfigMonitor.cpp
        ConfigMonitor.h
        ScanProcessMonitor.cpp
        ScanProcessMonitor.h
        Logger.cpp
        Logger.h
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${log4cpluslib}
        )

