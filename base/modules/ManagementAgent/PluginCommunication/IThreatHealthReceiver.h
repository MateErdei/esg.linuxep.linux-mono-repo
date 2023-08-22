// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/PluginApi/StatusInfo.h"
#include "ManagementAgent/HealthStatusImpl/HealthStatus.h"

#include <memory>
#include <string>

namespace ManagementAgent::PluginCommunication
{
    class IThreatHealthReceiver
    {
    public:
        virtual ~IThreatHealthReceiver() = default;

        /**
         * Handle plugin sending threat health to Management Agent
         *
         * @param pluginName
         * @param threatHealth Threat health in JSON format detailed in IBaseServiceApi.h
         */
        virtual bool receivedThreatHealth(
            const std::string& pluginName,
            const std::string& threatHealth,
            std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus>
                healthStatusSharedObj) = 0;
    };
} // namespace ManagementAgent::PluginCommunication
