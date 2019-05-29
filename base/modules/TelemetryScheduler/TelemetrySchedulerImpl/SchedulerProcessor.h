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
        SchedulerProcessor(std::shared_ptr<TaskQueue> taskQueue, std::shared_ptr<PluginCallback> pluginCallback);

        void run();

    private:
        std::shared_ptr<TaskQueue> m_taskQueue;
        std::shared_ptr<PluginCallback> m_pluginCallback;
    };
} // namespace TelemetrySchedulerImpl
