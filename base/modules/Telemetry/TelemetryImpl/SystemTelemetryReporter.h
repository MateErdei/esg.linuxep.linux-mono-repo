/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemTelemetryCollector.h"
#include "ITelemetryProvider.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelper/ITelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <utility>
#include <variant>

namespace Telemetry
{
    class SystemTelemetryReporter : public ITelemetryProvider
    {
    public:
        SystemTelemetryReporter(const ISystemTelemetryCollector& systemTelemetryCollector);

        std::string getName() override { return "system-telemetry"; }
        std::string getTelemetry() override;

    private:
        void getSimpleTelemetry(
            Common::Telemetry::TelemetryHelper& jsonConverter,
            const std::map<std::string, TelemetryItem>&
            systemTelemetryObjects);

        void getArraysTelemetry(
            Common::Telemetry::TelemetryHelper& jsonConverter,
            const std::map<std::string, std::vector<TelemetryItem>>&
            systemTelemetryArrays);

        const ISystemTelemetryCollector& m_systemTelemetryCollector;
    };
} // namespace Telemetry
