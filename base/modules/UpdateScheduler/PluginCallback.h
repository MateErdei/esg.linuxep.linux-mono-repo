/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApi/IPluginCallbackApi.h>
#include "SchedulerTaskQueue.h"

namespace UpdateScheduler
{
    class SchedulerPluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<SchedulerTaskQueue> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
    public:

        explicit SchedulerPluginCallback( std::shared_ptr<SchedulerTaskQueue> task );

        void applyNewPolicy(const std::string &policyXml) override;

        void queueAction(const std::string &actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string &appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

    };
};

