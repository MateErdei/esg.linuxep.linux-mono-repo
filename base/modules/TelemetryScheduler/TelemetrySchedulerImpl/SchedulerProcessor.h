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
            const std::string& supplementaryConfigFilepath,
            const std::string& telemetryExeConfigFilepath);

        void run();

    private:
        void ProcessRunTelemetry();

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::string m_telemetryExeConfigFilepath;
        std::string m_supplementaryConfigFilepath;
        unsigned int m_interval;
    };
} // namespace TelemetrySchedulerImpl
