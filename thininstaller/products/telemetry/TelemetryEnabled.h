// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Logger.h"

#include <string>

inline bool isTelemetryEnabled(const std::string& url)
{
    if (url == "")
    {
        LOGERROR("No MCS URL in thin installer");
        return false;
    }
    auto pos = url.rfind("https://", 0);
    if (pos != 0)
    {
        LOGERROR("MCS URL does not start with https://");
        return false;
    }
    auto remainder = url.substr(std::string{"https://"}.size());
    pos = remainder.find("/");
    if (pos == std::string::npos)
    {
        LOGERROR("MCS URL doesn't have a slash");
        return false;
    }

    auto host = remainder.substr(0, pos);
    if (host.empty())
    {
        LOGERROR("MCS URL does not have a host");
        return false;
    }
    auto sophosus = host.rfind("sophos.us");
    if (sophosus != std::string::npos)
    {
        LOGDEBUG("Telemetry disabled for sophos.us");
        return false;
    }

    return true;
}
