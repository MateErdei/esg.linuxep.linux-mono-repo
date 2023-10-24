// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "QueueTask.h"

#include "Common/PluginApi/IPluginCallbackApi.h"

#include <atomic>

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;

    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        void applyNewPolicy(const std::string& policyXml) override;
        void applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml) override;

        void queueAction(const std::string& ) override;
        void queueActionWithCorrelation(const std::string& queryJson, const std::string& correlationId) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);
        void setOsqueryRunning(bool running);
        void setOsqueryShouldBeRunning(bool shouldBerunning);

        std::string getTelemetry() override;
        void initialiseTelemetry();

        std::string getHealth() override;

        void setRunning(bool running);
        bool isRunning();

    private:
        std::atomic_bool m_running = false;
        bool m_osqueryRunning;
        bool m_osqueryShouldBeRunning;
    };
}; // namespace Plugin
