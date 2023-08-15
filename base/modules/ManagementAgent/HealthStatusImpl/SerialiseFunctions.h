// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "ManagementAgent/LoggerImpl/Logger.h"
#include "ManagementAgent/PluginCommunication/PluginHealthStatus.h"

#include <json.hpp>

namespace ManagementAgent
{
    namespace HealthStatusImpl
    {
        using healthValue_t = PluginCommunication::PluginHealthStatus::healthValue_t;
        std::string serialiseThreatHealth(
            const std::map<std::string, PluginCommunication::PluginHealthStatus>& pluginThreatDetectionHealth);

        std::map<std::string, PluginCommunication::PluginHealthStatus> deserialiseThreatHealth(
            const std::string& pluginThreatHealthJsonString);
        std::pair<bool,std::string> compareAndUpdateOverallHealth(healthValue_t health,healthValue_t service,healthValue_t threatService,healthValue_t threat);
    } // namespace HealthStatusImpl
} // namespace ManagementAgent
