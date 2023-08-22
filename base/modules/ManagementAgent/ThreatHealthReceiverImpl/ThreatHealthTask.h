// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/TaskQueue/ITask.h"
#include "ManagementAgent/HealthStatusImpl/HealthStatus.h"

#include <memory>
#include <string>

namespace ManagementAgent::ThreatHealthReceiverImpl
{
    class ThreatHealthTask : public virtual Common::TaskQueue::ITask
    {
    public:
        ThreatHealthTask(
            std::string pluginName,
            unsigned int threatHealth,
            std::shared_ptr<HealthStatusImpl::HealthStatus> healthStatus);

        void run() override;

    private:
        std::string m_pluginName;
        unsigned int m_threatHealth;
        std::shared_ptr<HealthStatusImpl::HealthStatus> m_healthStatus;
    };
} // namespace ManagementAgent::ThreatHealthReceiverImpl
