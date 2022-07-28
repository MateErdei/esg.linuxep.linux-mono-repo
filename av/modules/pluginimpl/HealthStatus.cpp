// Copyright 2022, Sophos Limited.  All rights reserved.

#include "HealthStatus.h"


std::string_view Plugin::threatHealthToString(E_HEALTH_STATUS status)
{
    switch(status)
    {
        case Plugin::E_THREAT_HEALTH_STATUS_GOOD:
            return "good";
        case Plugin::E_THREAT_HEALTH_STATUS_SUSPICIOUS:
            return "suspicious";
        default:
            return "unknown";
    }
}
