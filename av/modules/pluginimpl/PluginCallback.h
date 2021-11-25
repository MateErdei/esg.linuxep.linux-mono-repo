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
    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;

        int getServiceHealth();
        void setServiceHealth(int health);
        void calculateHealth();

        void sendStatus(const std::string& revID);
        void setRunning(bool running);
        bool isRunning();
        void setSXL4Lookups(bool sxl4Lookup);

    private:
        std::string generateSAVStatusXML();
        unsigned long getIdeCount();
        std::string getLrDataHash();
        std::string getMlLibHash();
        std::string getMlModelVersion();
        std::string getVirusDataVersion();

        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::string m_revID;
        std::atomic_bool m_running = false;
        bool m_lookupEnabled = true;
        int m_health = 1;
    };
}; // namespace Plugin
