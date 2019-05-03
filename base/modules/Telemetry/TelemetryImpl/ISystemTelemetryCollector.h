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
    class ISystemTelemetryCollector
    {
    public:
        virtual ~ISystemTelemetryCollector() = default;

        /// Collect all simple system telemetry items.
        /// \return Simple telemetry items, each indexed by the telemetry item's name.
        virtual std::map<std::string, std::vector<std::pair<std::string, std::variant<std::string, int>>>>
        collectObjects() const = 0;

        /// Collects array of telemetry objects.
        /// \return Telemetry item arrays, each indexed by the telemetry item's name.
        virtual std::map<std::string, std::vector<std::vector<std::pair<std::string, std::variant<std::string, int>>>>>
        collectArraysOfObjects() const = 0;
    };
} // namespace Telemetry
