/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TaskQueue.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

#include <atomic>

namespace TelemetrySchedulerImpl
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<TaskQueue> taskQueue);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;

        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

        bool shutdownReceived();

    private:
        std::atomic<bool> m_shutdownReceived;
        std::shared_ptr<TaskQueue> m_taskQueue;
        Common::PluginApi::StatusInfo m_statusInfo;
    };
} // namespace TelemetrySchedulerImpl
