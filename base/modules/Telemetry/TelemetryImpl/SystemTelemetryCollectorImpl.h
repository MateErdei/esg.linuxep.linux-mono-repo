/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemTelemetryCollector.h"
#include "SystemTelemetryConfig.h"

#include <Common/Process/IProcess.h>

#include <map>
#include <regex>
#include <string>
#include <variant>

namespace Telemetry
{
    class SystemTelemetryCollectorImpl : public ISystemTelemetryCollector
    {
    public:
        SystemTelemetryCollectorImpl(
            Telemetry::SystemTelemetryConfig objectsConfig,
            Telemetry::SystemTelemetryConfig arraysConfig);
        std::map<std::string, std::vector<std::variant<std::string, int>>> collectObjects() override;
        std::map<std::string, std::vector<std::vector<std::variant<std::string, int>>>> collectArraysOfObjects()
            override;

    protected:
        bool getValues(
            std::vector<std::variant<std::string, int>>& values,
            std::string& commandOutput,
            const std::string& regexp,
            std::vector<TelemetryValueType> valueTypes);

        bool getValues(
            std::vector<std::vector<std::variant<std::string, int>>>& values,
            std::string& commandOutput,
            const std::string& regexp,
            std::vector<TelemetryValueType> valueTypes);

    private:
        template<typename T>
        std::map<std::string, T> collect(const SystemTelemetryConfig& config);

        std::string getTelemetryItem(const std::string& command, const std::string& args) const;
        std::vector<std::string> matchSingleLine(std::istringstream& stream, const std::regex& re) const;

        Telemetry::SystemTelemetryConfig m_objectsConfig;
        Telemetry::SystemTelemetryConfig m_arraysConfig;
        mutable std::map<std::string, std::string> m_commandOutputCache;
    };
} // namespace Telemetry
