// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ITelemetryProvider.h"

#include "Common/PluginCommunication/IPluginProxy.h"

#include <memory>

namespace Telemetry
{
    /**
     * Class for reporting telemetry as JSON for a specific plugin.
     */
    class PluginTelemetryReporter : public ITelemetryProvider
    {
    public:
        explicit PluginTelemetryReporter(std::unique_ptr<Common::PluginCommunication::IPluginProxy> pluginProxy);

        std::string getName() override;
        std::string getTelemetry() override;

    private:
        std::unique_ptr<Common::PluginCommunication::IPluginProxy> m_pluginProxy;
    };

    class PluginTelemetryReporterWithoutIPC : public ITelemetryProvider
    {
    public:
        explicit PluginTelemetryReporterWithoutIPC(std::string pluginName);

        std::string getName() override;
        std::string getTelemetry() override;

    private:
        std::string m_pluginName;
    };

} // namespace Telemetry
