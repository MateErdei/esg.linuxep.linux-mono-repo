add_library(updatescheduler SHARED
        UpdatePolicyTranslator.cpp
        UpdatePolicyTranslator.h
        UpdateActionParser.cpp
        UpdateActionParser.h
        SulDownloaderRunner.cpp
        SulDownloaderRunner.h
        SchedulerTaskQueue.cpp
        SchedulerTaskQueue.h
        SulDownloaderResultDirectoryListener.cpp
        SulDownloaderResultDirectoryListener.h
        CronSchedulerThread.h
        CronSchedulerThread.cpp
        DownloadReportsAnalyser.cpp
        DownloadReportsAnalyser.h
        UpdateStatus.h
        UpdateStatus.cpp UpdateEvent.cpp UpdateEvent.h IMapHostCacheId.h)

target_link_libraries(updatescheduler PUBLIC suldownloaderimpl filesystemimpl utilityimpl applicationconfigurationimpl xmlutilities suldownloaderdata directorywatcherimpl)

install(TARGETS updatescheduler
        LIBRARY DESTINATION files/base/lib64)


install(TARGETS updatescheduler
        LIBRARY DESTINATION files/base/lib64)