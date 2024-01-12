// Copyright 2021-2024 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace ManagementAgent::PluginCommunication
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
        /**
         * Integer values from
         * https://sophos.atlassian.net/wiki/spaces/SophosCloud/pages/42132014794/EMP+status-health
         * For detail values:
         *  0 running
         *  1 not running
         *  2 missing
         *
         *  For summaries:
         *  1 good
         *  2 suspicious
         *  3 bad
         *
         */
        using healthValue_t = unsigned int;
        PluginHealthStatus() : healthType(HealthType::NONE), healthValue(0) {}

        bool operator==(const PluginHealthStatus& rhs) const
        {
            return this->healthValue == rhs.healthValue && this->displayName == rhs.displayName &&
                   this->healthValue == rhs.healthValue && this->activeHeartbeat == rhs.activeHeartbeat &&
                   this->activeHeartbeatUtmId == rhs.activeHeartbeatUtmId;
        }

        std::string displayName;
        HealthType healthType;
        healthValue_t healthValue;
        std::string activeHeartbeatUtmId = "";
        bool isolated = false;
        bool activeHeartbeat = false;
    };
} // namespace ManagementAgent::PluginCommunication
