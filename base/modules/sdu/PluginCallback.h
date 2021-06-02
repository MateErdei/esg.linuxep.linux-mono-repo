/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITaskQueue.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

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

    private:
        std::shared_ptr<ITaskQueue> m_taskQueue;

        std::string noPolicySetStatus{
                R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status version="1.0.0" is_running="1" />)sophos"
        };

        Common::PluginApi::StatusInfo m_statusInfo =
        Common::PluginApi::StatusInfo{ noPolicySetStatus, noPolicySetStatus, "SDU" };
    };
} // namespace RemoteDiagnoseImpl
