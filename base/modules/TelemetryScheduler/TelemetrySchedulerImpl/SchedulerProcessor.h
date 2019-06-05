/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "SleepyThread.h"
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
            std::shared_ptr<ITaskQueue> taskQueue,
            const Common::ApplicationConfiguration::IApplicationPathManager& pathManager);

        /**
         * Start the processor's main loop, processing tasks until Task::Shutdown is received.
         */
        virtual void run();

    protected:
        virtual void waitToRunTelemetry();
        virtual void runTelemetry();
        virtual void checkExecutableFinished();

        virtual bool delayingTelemetryRun()
        {
            return m_delayBeforeRunningTelemetry && !m_delayBeforeRunningTelemetry->finished();
        }

        virtual bool delayingTelemetryExeCheck()
        {
            return m_delayBeforeCheckingExe && !m_delayBeforeCheckingExe->finished();
        }

        virtual bool delayingConfigurationCheck()
        {
            return m_delayBeforeCheckingConfiguration && !m_delayBeforeCheckingConfiguration->finished();
        }

        size_t getIntervalFromSupplementaryFile();
        size_t getScheduledTimeUsingSupplementaryFile();

    private:
        std::shared_ptr<ITaskQueue> m_taskQueue;
        const Common::ApplicationConfiguration::IApplicationPathManager& m_pathManager;
        std::unique_ptr<SleepyThread> m_delayBeforeRunningTelemetry;
        std::unique_ptr<SleepyThread> m_delayBeforeCheckingConfiguration;
        std::unique_ptr<SleepyThread> m_delayBeforeCheckingExe;
        std::unique_ptr<Common::Process::IProcess> m_telemetryExeProcess;
        ;
    };
} // namespace TelemetrySchedulerImpl
