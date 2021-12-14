///////////////////////////////////////////////////////////
// Copyright (C) 2021 Sophos Plc, Oxford, England.
// All rights reserved.
///////////////////////////////////////////////////////////

#pragma once

#include <Common/PluginApi/StatusInfo.h>
#include <ManagementAgent/HealthStatusImpl/HealthStatus.h>

#include <memory>
#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IThreatHealthReceiver
        {
        public:
            virtual ~IThreatHealthReceiver() = default;

            /**
             * Handle plugin sending threat health to management agent
             *
             * @param pluginName
             * @param threatHealth Threat health in JSON TODO format?
             */
            virtual void receivedThreatHealth(
                const std::string& pluginName,
                const std::string& threatHealth,
                std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus>
                    healthStatusSharedObj) = 0;
        };
    } // namespace PluginCommunication
} // namespace ManagementAgent
