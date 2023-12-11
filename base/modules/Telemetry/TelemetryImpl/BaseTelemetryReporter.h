// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ITelemetryProvider.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <utility>
#include <variant>

#ifndef TEST_PUBLIC
#    define TEST_PUBLIC private
#endif

namespace Telemetry
{
    std::optional<nlohmann::json> parseOutbreakStatusFile();
    std::optional<std::string> extractValueFromFile(const Path& filePath, const std::string& key);
    std::optional<std::string> extractCustomerId(const std::string& policyXml);
    std::optional<std::string> extractOverallHealth(const std::string& shsStatusXml);
    /**
     * Class for reporting system telemetry as JSON for base
     */
    class BaseTelemetryReporter : public ITelemetryProvider
    {
    public:
        BaseTelemetryReporter() = default;

        std::string getName() override { return "base-telemetry"; }
        std::string getTelemetry() override;

        TEST_PUBLIC : using clock_t = std::chrono::system_clock;
        using time_point_t = std::chrono::time_point<clock_t>;
        static std::optional<std::string> getOutbreakModeCurrent();
        static std::optional<std::string> getOutbreakModeToday(time_point_t now);
        static std::optional<std::string> getOutbreakModeHistoric();

    private:
        static std::optional<std::string> getCustomerId();
        static std::optional<std::string> getEndpointId();
        static std::optional<std::string> getTenantId();
        static std::optional<std::string> getDeviceId();
        static std::optional<std::string> getMachineId();
        static std::optional<std::string> getVersion();
        static std::optional<std::string> getOverallHealth();
        static std::optional<std::string> getOutbreakModeTodayWrapper();
    };
} // namespace Telemetry
