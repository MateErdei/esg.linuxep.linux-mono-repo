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
     * Type representing telemetry item name, command, command-arguments, regex and property.
     */
    using SystemTelemetryTuple =
        std::tuple<const std::string, const std::string, const std::string, std::vector<TelemetryProperty>>;

    using SystemTelemetryConfig = std::map<const std::string, const SystemTelemetryTuple>;

    extern const SystemTelemetryConfig GL_systemTelemetryObjectsConfig;
    extern const SystemTelemetryConfig GL_systemTelemetryArraysConfig;
} // namespace Telemetry
