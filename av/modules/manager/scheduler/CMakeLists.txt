CAPN_GENERATE_CPP(CapNGeneratedFilesSrcs CapNGeneratedFilesHeaders
        NamedScan.capnp)

sophos_add_library(scanscheduler SHARED
        ${CapNGeneratedFilesHeaders}
        ${CapNGeneratedFilesSrcs}
        DaySet.cpp
        DaySet.h
        GlobalConfiguration.cpp
        GlobalConfiguration.h
        Logger.cpp
        Logger.h
        ScanRunner.cpp
        ScanRunner.h
        ScanScheduler.cpp
        ScanScheduler.h
        ScheduledScan.cpp
        ScheduledScan.h
        ScheduledScanConfiguration.cpp
        ScheduledScanConfiguration.h
        TimeSet.cpp
        TimeSet.h
        EXTRA_INCLUDES "${CAPNPROTO_INCLUDE_DIR}" ${CMAKE_CURRENT_BINARY_DIR}
        )
