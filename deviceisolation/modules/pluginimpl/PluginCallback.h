// Copyright 2023-2024 Sophos Limited. All rights reserved.

#pragma once

#include "TaskQueue.h"

#include "Common/PluginApi/IPluginCallbackApi.h"

#include <atomic>

namespace Plugin
{
    class PluginCallback : public Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<TaskQueue> m_task;

    public:
        explicit PluginCallback(std::shared_ptr<TaskQueue> task);

        void applyNewPolicy(const std::string& policyXml) override;
        void applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml) override;

        void queueActionWithCorrelation(const std::string& content, const std::string& correlationId) override;
        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;
        virtual uint getHealthInner();

        void setRunning(bool running);
        bool isRunning();
        virtual void setIsolated(const bool isolated);

    private:
        std::atomic_bool m_running = false;
        std::atomic_bool isolated_ = false;
    };
}; // namespace Plugin
