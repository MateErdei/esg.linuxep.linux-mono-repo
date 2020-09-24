/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"

#include <Common/PluginApi/IPluginCallbackApi.h>

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;

        void sendStatus(const std::string& revID);

    private:
        std::string generateSAVStatusXML();
        unsigned long getIdeCount();
        std::string getLrDataHash();
        std::string getMlLibHash();
        std::string getPluginVersion();
        std::string getVirusDataVersion();

        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::string m_revID;
    };
}; // namespace Plugin
