/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemTelemetryCollector.h"
#include "ITelemetryProvider.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>

#include <utility>
#include <variant>

namespace Telemetry
{
    /**
     * Class for reporting telemetry as JSON for a specific plugin.
     */
    class PluginTelemetryReporter : public ITelemetryProvider
    {
    public:
        explicit PluginTelemetryReporter(
            std::shared_ptr<ManagementAgent::PluginCommunication::IPluginManager> pluginManager,
            const std::string& pluginName);

        std::string getName() override;
        std::string getTelemetry() override;

        std::shared_ptr<ManagementAgent::PluginCommunication::IPluginManager> m_pluginManager;
        const std::string m_pluginName;
    };
} // namespace Telemetry
