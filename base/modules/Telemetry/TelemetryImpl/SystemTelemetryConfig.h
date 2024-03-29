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

    struct TelemetryProperty
    {
        std::string name;
        TelemetryValueType type;
    };

    /**
     * Type representing command to get telemetry data, command-arguments, regex and property.
     */
    using SystemTelemetryTuple =
        std::tuple<const std::string, std::vector<std::string>, const std::string, std::vector<TelemetryProperty>>;

    /**
     * Type representing telemetry item name and how to extract the telemetry.
     */
    using SystemTelemetryConfig = std::map<const std::string, const SystemTelemetryTuple>;

    SystemTelemetryConfig systemTelemetryObjectsConfig();
    SystemTelemetryConfig systemTelemetryArraysConfig();
} // namespace Telemetry
