/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

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

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

        void setRunning(bool running);
        bool isRunning();
        void setSXL4Lookups(bool sxl4Lookup);

    private:
        std::atomic_bool m_running = false;
        bool m_lookupEnabled = true;
    };
}; // namespace Plugin
