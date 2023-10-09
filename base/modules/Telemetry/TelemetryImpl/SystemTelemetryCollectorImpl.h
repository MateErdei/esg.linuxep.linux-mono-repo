// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISystemTelemetryCollector.h"
#include "SystemTelemetryConfig.h"

#include "Common/Process/IProcess.h"

#include <map>
#include <regex>
#include <string>
#include <variant>

namespace Telemetry
{
    /**
     * Class for collecting system telemetry into an internal data structure.
     */
    class SystemTelemetryCollectorImpl : public ISystemTelemetryCollector
    {
    public:
        SystemTelemetryCollectorImpl(
            Telemetry::SystemTelemetryConfig objectsConfig,
            Telemetry::SystemTelemetryConfig arraysConfig,
            unsigned waitTimeMilliSeconds = 100,
            unsigned waitMaxRetries = 10);

        std::map<std::string, TelemetryItem> collectObjects() const override;

        std::map<std::string, std::vector<TelemetryItem>> collectArraysOfObjects() const override;

    protected:
        bool getTelemetryValuesFromCommandOutput(
            TelemetryItem& values,
            std::string& commandOutput,
            const std::string& regexp,
            std::vector<TelemetryProperty> properties) const;

        bool getTelemetryValuesFromCommandOutput(
            std::vector<TelemetryItem>& values,
            std::string& commandOutput,
            const std::string& regexp,
            std::vector<TelemetryProperty> properties) const;

    private:
        template<typename T>
        std::map<std::string, T> collect(const SystemTelemetryConfig& config) const;

        std::string getTelemetryItem(const std::string& command, std::vector<std::string> args) const;
        std::vector<std::string> matchSingleLine(std::istringstream& stream, const std::regex& re) const;
        std::string getSystemCommandExecutablePath(const std::string& executableName) const;

        Telemetry::SystemTelemetryConfig m_objectsConfig;
        Telemetry::SystemTelemetryConfig m_arraysConfig;
        unsigned m_waitTimeMilliSeconds;
        unsigned m_waitMaxRetries;
        mutable std::map<std::string, std::string> m_commandOutputCache;
    };
} // namespace Telemetry
