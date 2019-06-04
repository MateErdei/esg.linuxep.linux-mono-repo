/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "TaskQueue.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <Common/Process/IProcess.h>

#include <atomic>

namespace TelemetrySchedulerImpl
{
    class SchedulerProcessor
    {
    public:
        SchedulerProcessor(
            std::shared_ptr<TaskQueue> taskQueue,
            const Common::ApplicationConfiguration::IApplicationPathManager& pathManager);

        /**
         * Start the processor's main loop, processing tasks until Task::Shutdown is received.
         */
        virtual void run();

    protected:
        virtual void waitToRunTelemetry();
        virtual void runTelemetry();
        virtual void checkExecutableFinished();

        virtual bool delayingTelemetryRun() { return m_delayBeforeRunningTelemetryState; }
        virtual bool delayingConfigurationCheck() { return m_delayBeforeCheckingConfigurationState; }

        size_t getIntervalFromSupplementaryFile();
        size_t getScheduledTimeUsingSupplementaryFile();
        void delayBeforeQueueingTask(size_t delayInSeconds, std::atomic<bool>& runningState, Task task);

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        const Common::ApplicationConfiguration::IApplicationPathManager& m_pathManager;
        std::atomic<bool> m_delayBeforeRunningTelemetryState;
        std::atomic<bool> m_delayBeforeCheckingConfigurationState;
        std::atomic<bool> m_delayBeforeCheckingExeState;
        std::unique_ptr<Common::Process::IProcess> m_telemetryExeProcess;
    };
} // namespace TelemetrySchedulerImpl
