/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginTelemetryReporter.h"

#include "TelemetryProcessor.h"

#include <Telemetry/LoggerImpl/Logger.h>

namespace Telemetry
{
    PluginTelemetryReporter::PluginTelemetryReporter(
        std::unique_ptr<Common::PluginCommunication::IPluginProxy> pluginProxy) :
        m_pluginProxy(std::move(pluginProxy))
    {
    }

    std::string PluginTelemetryReporter::getName() { return m_pluginProxy->name(); }

    std::string PluginTelemetryReporter::getTelemetry() { return m_pluginProxy->getTelemetry(); }

    PluginTelemetryReporterWithoutIPC::PluginTelemetryReporterWithoutIPC(std::string pluginName) :
        m_pluginName(std::move(pluginName))
    {
    }

    std::string PluginTelemetryReporterWithoutIPC::getName() { return m_pluginName; }

    std::string PluginTelemetryReporterWithoutIPC::getTelemetry() { return "{}"; }

} // namespace Telemetry