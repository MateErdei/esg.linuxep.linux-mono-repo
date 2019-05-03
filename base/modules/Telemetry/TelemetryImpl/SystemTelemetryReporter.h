/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemTelemetryCollector.h"
#include "json.hpp"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelper/ITelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <utility>
#include <variant>

namespace Telemetry
{
    std::string gatherSystemTelemetry(ISystemTelemetryCollector& systemTelemetryCollector);

    void getSimpleTelemetry(
        Common::Telemetry::TelemetryHelper& jsonConverter,
        const std::map<std::string, std::vector<std::pair<std::string, std::variant<std::string, int>>>>&
            systemTelemetryObjects);

    void getArraysTelemetry(
        Common::Telemetry::TelemetryHelper& jsonConverter,
        const std::map<std::string, std::vector<std::vector<std::pair<std::string, std::variant<std::string, int>>>>>&
            systemTelemetryArrays);
} // namespace Telemetry
