/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITelemetryProvider.h"

#include <Common/FileSystem/IFileSystem.h>

#include <optional>
#include <utility>
#include <variant>

namespace Telemetry
{
    std::optional<std::string> extractValueFromFile(const Path& filePath, const std::string& key);
    std::optional<std::string> extractCustomerId(const std::string& policyXml);
    /**
     * Class for reporting system telemetry as JSON for base
     */
    class BaseTelemetryReporter : public ITelemetryProvider
    {
    public:
        BaseTelemetryReporter() = default;

        std::string getName() override { return "base-telemetry"; }
        std::string getTelemetry() override;

    private:
        static std::optional<std::string> getCustomerId();
        static std::optional<std::string> getEndpointId();
        static std::optional<std::string> getMachineId();
        static std::optional<std::string> getVersion();
    };
} // namespace Telemetry
