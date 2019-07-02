/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITelemetryProvider.h"

#include <ManagementAgent/PluginCommunication/IPluginManager.h>

namespace Telemetry
{
    /**
     * Class for reporting telemetry as JSON for a specific plugin.
     */
    class PluginTelemetryReporter : public ITelemetryProvider
    {
    public:
        explicit PluginTelemetryReporter(
            std::unique_ptr<Common::PluginCommunication::IPluginProxy> pluginProxy);

        std::string getName() override;
        std::string getTelemetry() override;

        std::unique_ptr<Common::PluginCommunication::IPluginProxy> m_pluginProxy;
    };
} // namespace Telemetry
