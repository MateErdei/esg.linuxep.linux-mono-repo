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
            PluginHealthStatus() : healthType(HealthType::NONE), healthValue(0)
            {
            }

            HealthType healthType;
            std::string displayName;
            unsigned int healthValue;
        };
    }
}

