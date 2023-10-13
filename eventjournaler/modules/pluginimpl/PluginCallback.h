/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TaskQueue.h"

#include <Common/PluginApi/IPluginCallbackApi.h>
#include <modules/Heartbeat/IHeartbeat.h>

#include <atomic>

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<TaskQueue> m_task;

    public:
        PluginCallback(std::shared_ptr<TaskQueue> task, std::shared_ptr<Heartbeat::IHeartbeat> heartbeat);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;
        virtual uint getHealthInner();

        void setRunning(bool running);
        bool isRunning();
        static uint getAcceptableDailyDroppedEvents();
        static const uint ACCEPTABLE_DAILY_DROPPED_EVENTS = 5;

    private:
        std::atomic_bool m_running = false;
        std::shared_ptr<Heartbeat::IHeartbeat> m_heartbeat;
    };
}; // namespace Plugin
