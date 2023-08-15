// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ITaskQueue.h"

#include "Common/PluginApi/IPluginCallbackApi.h"

#include <atomic>
#include <memory>

namespace RemoteDiagnoseImpl
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<ITaskQueue> taskQueue);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;

        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

        std::string getHealth() override;

    private:
        std::shared_ptr<ITaskQueue> m_taskQueue;

        Common::PluginApi::StatusInfo m_statusInfo =
        Common::PluginApi::StatusInfo{};
    };
} // namespace RemoteDiagnoseImpl
