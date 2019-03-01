add_library(updateschedulerimpl SHARED
        configModule/UpdatePolicyTranslator.cpp
        configModule/UpdatePolicyTranslator.h
        configModule/UpdateActionParser.cpp
        configModule/UpdateActionParser.h
        configModule/DownloadReportsAnalyser.cpp
        configModule/DownloadReportsAnalyser.h
        configModule/PropertyTreeHelper.h
        configModule/UpdateStatus.h
        configModule/UpdateStatus.cpp
        configModule/UpdateEvent.cpp
        configModule/UpdateEvent.h
        cronModule/CronSchedulerThread.h
        cronModule/CronSchedulerThread.cpp
        runnerModule/SulDownloaderRunner.cpp
        runnerModule/SulDownloaderRunner.h
        runnerModule/SulDownloaderResultDirectoryListener.cpp
        runnerModule/SulDownloaderResultDirectoryListener.h
        runnerModule/SulDownloaderRunnerFactory.cpp
        runnerModule/SulDownloaderRunnerFactory.h
        runnerModule/AsyncSulDownloaderRunner.cpp
        runnerModule/AsyncSulDownloaderRunner.h
        ../UpdateScheduler/SchedulerTaskQueue.cpp
        ../UpdateScheduler/SchedulerTaskQueue.h
        ../UpdateScheduler/IMapHostCacheId.h
        ../UpdateScheduler/IAsyncSulDownloaderRunner.h
        ../UpdateScheduler/ICronSchedulerThread.h
        ../UpdateScheduler/ScheduledUpdate.cpp
        ../UpdateScheduler/ScheduledUpdate.h
        SchedulerPluginCallback.h
        SchedulerPluginCallback.cpp
        Logger.h
        Logger.cpp
        UpdateSchedulerProcessor.h
        UpdateSchedulerProcessor.cpp
        UpdateSchedulerBootstrap.h
        UpdateSchedulerBootstrap.cpp
        )

target_include_directories(updateschedulerimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

target_link_libraries(updateschedulerimpl PUBLIC
        filesystemimpl
        utilityimpl
        applicationconfigurationimpl
        xmlutilities
        suldownloaderdata
        directorywatcherimpl
        pluginapiimpl
        osutilitiesimpl
        ${LOG4CPLUS_LIBRARY} )

SET_TARGET_PROPERTIES( updateschedulerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS updateschedulerimpl
        LIBRARY DESTINATION files/base/lib64)
