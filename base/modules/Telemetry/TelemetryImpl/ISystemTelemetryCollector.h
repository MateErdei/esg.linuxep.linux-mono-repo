/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Telemetry
{
    // Type representing a telemetry item consisting of one or more key/value pairs:
    using TelemetryItem = std::vector<std::pair<std::string, std::variant<std::string, int>>>;

    class ISystemTelemetryCollector
    {
    public:
        virtual ~ISystemTelemetryCollector() = default;

        /// Collect all simple system telemetry items.
        /// \return Simple telemetry items, each indexed by the telemetry item's name.
        virtual std::map<std::string, TelemetryItem> collectObjects() const = 0;

        /// Collects array of telemetry objects.
        /// \return Telemetry item arrays, each indexed by the telemetry item's name.
        virtual std::map<std::string, std::vector<TelemetryItem>> collectArraysOfObjects() const = 0;
    };
} // namespace Telemetry
