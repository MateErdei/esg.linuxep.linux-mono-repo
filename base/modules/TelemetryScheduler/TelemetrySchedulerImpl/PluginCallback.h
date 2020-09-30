/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

#include <atomic>
#include <memory>

namespace TelemetrySchedulerImpl
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<ITaskQueue> taskQueue);

        void applyNewPolicy(const std::string& appId, const std::string& policyContent) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;

        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;

    private:
        std::shared_ptr<ITaskQueue> m_taskQueue;
    };
} // namespace TelemetrySchedulerImpl
