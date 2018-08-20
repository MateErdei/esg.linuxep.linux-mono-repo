add_library(updateschedulerimpl SHARED
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
        UpdateStatus.cpp
        UpdateEvent.cpp
        UpdateEvent.h
        IMapHostCacheId.h
        SchedulerPluginCallback.h
        SchedulerPluginCallback.cpp
        Logger.h
        LoggingSetup.h
        LoggingSetup.cpp
        UpdateScheduler.h
        UpdateScheduler.cpp
        UpdateSchedulerBootstrap.h
        UpdateSchedulerBootstrap.cpp
        ISulDownloaderRunner.h)

target_include_directories(updateschedulerimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(updateschedulerimpl PUBLIC
        suldownloaderimpl
        filesystemimpl
        utilityimpl
        applicationconfigurationimpl
        xmlutilities
        suldownloaderdata
        directorywatcherimpl
        pluginapiimpl
        ${LOG4CPLUS_LIBRARY} )

SET_TARGET_PROPERTIES( updateschedulerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS updateschedulerimpl
        LIBRARY DESTINATION files/base/lib64)
