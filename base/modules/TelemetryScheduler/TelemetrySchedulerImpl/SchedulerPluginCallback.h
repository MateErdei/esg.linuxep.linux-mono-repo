/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginCallbackApi.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <atomic>

namespace TelemetrySchedulerImpl
{
    class SchedulerPluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit SchedulerPluginCallback(); // TODO: pass in task queue - see UpdateScheduler for example

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;

        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

        bool shutdownReceived();

    private:
        Common::PluginApi::StatusInfo m_statusInfo;
        std::atomic<bool> m_shutdownReceived;
    };
}; // namespace TelemetrySchedulerImpl
