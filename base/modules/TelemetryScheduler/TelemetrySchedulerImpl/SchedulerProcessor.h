/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "TaskQueue.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <atomic>

namespace TelemetrySchedulerImpl
{
    class SchedulerProcessor
    {
    public:
        SchedulerProcessor(
            std::shared_ptr<TaskQueue> taskQueue,
            const Common::ApplicationConfiguration::IApplicationPathManager& pathManager);

        virtual void run();

        bool delayingTelemetryRun() { return m_delayBeforeRunningTelemetryState; }
        bool delayingConfigurationCheck() { return m_delayBeforeCheckingConfigurationState; }

    protected:
        virtual void waitToRunTelemetry();
        virtual void runTelemetry();
        size_t getIntervalFromSupplementaryFile();
        size_t getScheduledTimeUsingSupplementaryFile();
        void delayBeforeQueueingTask(size_t delayInSeconds, std::atomic<bool>& runningState, Task task);

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        const Common::ApplicationConfiguration::IApplicationPathManager& m_pathManager;
        std::atomic<bool> m_delayBeforeRunningTelemetryState;
        std::atomic<bool> m_delayBeforeCheckingConfigurationState;
    };
} // namespace TelemetrySchedulerImpl
