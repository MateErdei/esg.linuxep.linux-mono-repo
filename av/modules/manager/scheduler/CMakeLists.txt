
sophos_add_library(scanscheduler SHARED
        DaySet.cpp
        DaySet.h
        GlobalConfiguration.cpp
        GlobalConfiguration.h
        IScanComplete.h
        Logger.cpp
        Logger.h
        ScanRunner.cpp
        ScanRunner.h
        ScanScheduler.cpp
        ScanScheduler.h
        ScanSerialiser.cpp
        ScanSerialiser.h
        ScheduledScan.cpp
        ScheduledScan.h
        ScheduledScanConfiguration.cpp
        ScheduledScanConfiguration.h
        TimeSet.cpp
        TimeSet.h
        EXTRA_LIBS ${STD_FILESYSTEM_IF_REQUIRED}  scanmessages
        EXTRA_INCLUDES "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        )