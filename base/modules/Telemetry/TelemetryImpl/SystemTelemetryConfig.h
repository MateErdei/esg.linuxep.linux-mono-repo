/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace Telemetry
{
    enum class TelemetryValueType
    {
        STRING,
        INTEGER
    };

    // telemetry item name to command, command-arguments, regex and type-of-value
    using SystemTelemetryTuple =
        std::tuple<const std::string, const std::string, const std::string, std::vector<TelemetryValueType>>;

    using SystemTelemetryConfig = std::map<const std::string, const SystemTelemetryTuple>;

    extern const SystemTelemetryConfig GL_systemTelemetryObjectsConfig;
    extern const SystemTelemetryConfig GL_systemTelemetryArraysConfig;
} // namespace Telemetry
