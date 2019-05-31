/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "TaskQueue.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginCallbackApi.h>

#include <atomic>

namespace TelemetrySchedulerImpl
{
    class SchedulerProcessor
    {
    public:
        SchedulerProcessor(
            std::shared_ptr<TaskQueue> taskQueue,
            const std::string& supplementaryConfigFilePath,
            const std::string& telemetryExeConfigFilePath,
            const std::string& telemetryStatusFilePath);

        void run();

    private:
        void waitToRunTelemetry();
        void runTelemetry();
        void scheduleTelemetryRunFromSupplementaryFile();
        size_t getIntervalFromSupplementaryFile();
        size_t getScheduledTimeUsingSupplementaryFile();

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::string m_telemetryExeConfigFilePath;
        std::string m_supplementaryConfigFilePath;
        std::string m_telemetryStatusFilePath;
    };
} // namespace TelemetrySchedulerImpl
