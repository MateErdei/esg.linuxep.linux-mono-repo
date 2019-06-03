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

    protected:
        virtual void waitToRunTelemetry();
        virtual void runTelemetry();
        virtual size_t getIntervalFromSupplementaryFile();
        virtual size_t getScheduledTimeUsingSupplementaryFile();

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        const Common::ApplicationConfiguration::IApplicationPathManager& m_pathManager;
    };
} // namespace TelemetrySchedulerImpl
