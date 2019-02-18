/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginCallbackApi.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <atomic>

namespace UpdateSchedulerImpl
{
    class SchedulerPluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::atomic<bool> m_shutdownReceived;

    public:
        explicit SchedulerPluginCallback(std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> task);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;
        bool shutdownReceived();
    };
}; // namespace UpdateSchedulerImpl
