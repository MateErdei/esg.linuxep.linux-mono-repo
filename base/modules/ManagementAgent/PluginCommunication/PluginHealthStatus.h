/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        enum class HealthType
        {
            NONE,
            SERVICE,
            THREAT_SERVICE,
            SERVICE_AND_THREAT,
            THREAT_DETECTION
        };

        struct PluginHealthStatus
        {
            PluginHealthStatus() : healthType(HealthType::NONE), healthValue(0) {}

            bool operator==(const PluginHealthStatus& rhs) const
            {
                return this->healthValue == rhs.healthValue && this->displayName == rhs.displayName &&
                       this->healthValue == rhs.healthValue && this->activeHeartbeat == rhs.activeHeartbeat && this->activeHeartbeatUtmId == rhs.activeHeartbeatUtmId;
            }

            HealthType healthType;
            std::string displayName;
            unsigned int healthValue;
            std::string activeHeartbeatUtmId = "";
            bool activeHeartbeat = false;
        };

    } // namespace PluginCommunication
} // namespace ManagementAgent
