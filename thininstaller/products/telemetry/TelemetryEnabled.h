// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Datatypes/Print.h"

#include <string>

inline bool isTelemetryEnabled(const std::string& url)
{
    if (url == "")
    {
        PRINT("No MCS URL in thin installer");
        return false;
    }
    auto pos = url.rfind("https://", 0);
    if (pos != 0)
    {
        PRINT("MCS URL does not start with https://");
        return false;
    }
    auto remainder = url.substr(std::string{"https://"}.size());
    pos = remainder.find("/");
    if (pos == std::string::npos)
    {
        PRINT("MCS URL doesn't have a slash");
        return false;
    }

    auto host = remainder.substr(0, pos);
    if (host.empty())
    {
        PRINT("MCS URL does not have a host");
        return false;
    }
    auto sophosus = host.rfind("sophos.us");
    if (sophosus != std::string::npos)
    {
        return false;
    }

    return true;
}
