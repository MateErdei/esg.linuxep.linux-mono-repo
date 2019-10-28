/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"
#include "PluginCallback.h"
#include "SchedulerStatus.h"
#include "SleepyThread.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <Common/Process/IProcess.h>
#include <Common/TelemetryConfigImpl/Config.h>

#include <atomic>
#include <chrono>
#include <tuple>

namespace TelemetrySchedulerImpl
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    class SchedulerProcessor
    {
    public:
        SchedulerProcessor(
            std::shared_ptr<ITaskQueue> taskQueue,
            const Common::ApplicationConfiguration::IApplicationPathManager& pathManager,
            seconds configurationCheckDelay = 3600s,
            seconds telemetryExeCheckDelay = 60s);

        /**
         * Start the processor's main loop, processing tasks until Task::Shutdown is received.
         */
        virtual void run();

    protected:
        virtual void waitToRunTelemetry(bool runScheduledInPastNow);
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

        std::tuple<SchedulerStatus, bool> getStatusFromFile() const;
        std::tuple<Common::TelemetryConfigImpl::Config, bool> getConfigFromFile() const;

        void updateStatusFile(const system_clock::time_point& scheduledTime) const;

        system_clock::time_point getNextScheduledTime(
            system_clock::time_point previousScheduledTime,
            unsigned int intervalSeconds) const;

        void delayBeforeQueueingTask(
            std::chrono::system_clock::time_point delayUntil,
            std::unique_ptr<SleepyThread>& delayThread,
            SchedulerTask task);

        bool isTelemetryDisabled(
            const system_clock::time_point& previousScheduledTime,
            bool statusFileValid,
            unsigned int interval,
            bool configFileValid);

    private:
        std::shared_ptr<ITaskQueue> m_taskQueue;
        const Common::ApplicationConfiguration::IApplicationPathManager& m_pathManager;
        std::unique_ptr<SleepyThread> m_delayBeforeRunningTelemetry;
        std::unique_ptr<SleepyThread> m_delayBeforeCheckingConfiguration;
        std::unique_ptr<SleepyThread> m_delayBeforeCheckingExe;
        std::unique_ptr<Common::Process::IProcess> m_telemetryExeProcess;
        const seconds m_configurationCheckDelay;
        const seconds m_telemetryExeCheckDelay;
    };
} // namespace TelemetrySchedulerImpl
