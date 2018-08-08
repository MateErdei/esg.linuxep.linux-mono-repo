/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApiImpl/PluginCallBackHandler.h>
#include "QueueTask.h"

namespace UpdateScheduler
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
    public:

        explicit PluginCallback( std::shared_ptr<QueueTask> task );

        void applyNewPolicy(const std::string &policyXml) override;

        void queueAction(const std::string &actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string &appId) override;
        void setStatus(Common::PluginApi::StatusInfo statusInfo);

        std::string getTelemetry() override;

    };
};

