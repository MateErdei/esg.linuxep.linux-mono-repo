/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"

#include <Common/PluginApi/IPluginCallbackApi.h>
#include <mutex>

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::once_flag m_restoreTelemetryFromDiskFlag;

    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        // not implemented.
        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& ) override;
        void queueActionWithCorrelation(const std::string& queryJson, const std::string& correlationId) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;
        void initialiseTelemetry();
        void saveTelemetry() override;
    };
}; // namespace Plugin
