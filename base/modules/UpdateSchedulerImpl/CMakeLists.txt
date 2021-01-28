include(stateMachinesModule/proto/CMakeLists.txt)

add_library(updateschedulerimpl SHARED
        ${STATE_MACHINE_DATA_PROTO_SRC}
        ${STATE_MACHINE_DATA_PROTO_H}
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
        cronModule/Logger.cpp
        cronModule/Logger.h
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
        SchedulerPluginCallback.h
        SchedulerPluginCallback.cpp
        Logger.h
        Logger.cpp
        UpdateSchedulerProcessor.h
        UpdateSchedulerProcessor.cpp
        UpdateSchedulerBootstrap.h
        UpdateSchedulerBootstrap.cpp
        ../UpdateScheduler/ICacheEvaluator.h
        ../UpdateScheduler/IHealthEventManager.h
        ../UpdateScheduler/IState.h
        ../UpdateScheduler/IVersionPersister.h
        stateMachinesModule/DownloadStateMachine.h
        stateMachinesModule/DownloadStateMachine.cpp
        stateMachinesModule/EventStateMachine.h
        stateMachinesModule/EventStateMachine.cpp
        stateMachinesModule/InstallStateMachine.h
        stateMachinesModule/InstallStateMachine.cpp
        stateMachinesModule/State.h
        stateMachinesModule/State.cpp
        stateMachinesModule/TieWithCheck.h
        stateMachinesModule/SEUErrors.h
        stateMachinesModule/Logger.h
        stateMachinesModule/Logger.cpp
        stateMachinesModule/StateMachineException.h
        stateMachinesModule/StateMachineException.cpp
        stateMachinesModule/StateMachineData.h
        stateMachinesModule/StateMachineData.cpp
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
        telemetryhelperimpl
        watchdogservice
        ${LOG4CPLUS_LIBRARY}
        protobufutil)

SET_TARGET_PROPERTIES( updateschedulerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS updateschedulerimpl
        LIBRARY DESTINATION files/base/lib64)
