add_subdirectory(common)

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
        common/Logger.h
        common/Logger.cpp
        UpdateSchedulerProcessor.h
        UpdateSchedulerProcessor.cpp
        UpdateSchedulerBootstrap.h
        UpdateSchedulerBootstrap.cpp
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
        common/StateMachineException.h
        common/StateMachineException.cpp
        common/StateMachineData.h
        common/StateMachineData.cpp
        stateMachinesModule/StateMachineProcessor.cpp
        stateMachinesModule/StateMachineProcessor.h
        UpdateSchedulerUtils.cpp
        UpdateSchedulerUtils.h
        UpdateSupplementDecider.cpp
        UpdateSupplementDecider.h
        )

target_include_directories(updateschedulerimpl SYSTEM PUBLIC
        ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR}
        ${AUTOVERSION_INCLUDE}
        ${CMAKE_BINARY_DIR}/modules
        ${PROTOBUF_INCLUDE_DIR}
        )

target_link_libraries(updateschedulerimpl PUBLIC
        StateMachineDataProto
        filesystemimpl
        flagutilsobject
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
        protobufutil
        procutilimpl
        policy
        downloadreport
        updateutilities
        )

SET_TARGET_PROPERTIES( updateschedulerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS updateschedulerimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
