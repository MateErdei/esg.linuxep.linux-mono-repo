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
        static std::optional<std::string> extractValueFromIniFile(const Path &filePath, const std::string &key);
        static std::optional<std::string> extractCustomerId(const std::string& policyXml);
    };
} // namespace Telemetry
