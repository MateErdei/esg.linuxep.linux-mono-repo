/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/LoggerImpl/Logger.h>
#include <ManagementAgent/PluginCommunication/PluginHealthStatus.h>

#include <json.hpp>

namespace ManagementAgent
{
    namespace HealthStatusImpl
    {
        std::string serialiseThreatHealth(
            const std::map<std::string, PluginCommunication::PluginHealthStatus>& pluginThreatDetectionHealth);

        std::map<std::string, PluginCommunication::PluginHealthStatus> deserialiseThreatHealth(
            const std::string& pluginThreatHealthJsonString);

    } // namespace HealthStatusImpl
} // namespace ManagementAgent
