// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "QueueTask.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

#include <atomic>

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<QueueTask> m_task;

    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;

        void setRunning(bool running);
        bool isRunning();

    private:
        std::atomic_bool m_running = false;
    };
}; // namespace Plugin
