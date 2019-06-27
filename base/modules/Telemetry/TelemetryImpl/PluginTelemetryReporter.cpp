/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginTelemetryReporter.h"

#include "TelemetryProcessor.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>

namespace Telemetry
{
    PluginTelemetryReporter::PluginTelemetryReporter(
        std::shared_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager,
        const std::string& pluginName) : m_pluginManager(pluginManager), m_pluginName(pluginName)
    {
    }

    std::string PluginTelemetryReporter::getName() { return m_pluginName; }

    std::string PluginTelemetryReporter::getTelemetry() { return m_pluginManager->getTelemetry(m_pluginName); }
}
