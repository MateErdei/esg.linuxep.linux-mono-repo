/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <atomic>

namespace UpdateSchedulerImpl
{
    class SchedulerPluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::atomic<bool> m_shutdownReceived;

    public:
        explicit SchedulerPluginCallback(std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> task);

        void applyNewPolicy(const std::string& policyXml) override;
        void applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;
        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);
        void setStateMachine(StateData::StateMachineData stateMachineData);

        std::string getTelemetry() override;
        std::string getHealth() override;
        bool shutdownReceived();

        void setRunning(bool running);
        bool isRunning();

    private:
        std::atomic_bool m_running = false;
        StateData::StateMachineData m_stateMachineData;
    };
} // namespace UpdateSchedulerImpl
