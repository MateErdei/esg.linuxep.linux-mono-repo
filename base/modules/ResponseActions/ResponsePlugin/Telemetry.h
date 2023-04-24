// Copyright 2023 Sophos Limited. All rights reserved.


#pragma once

#include <optional>
#include <string>

namespace ResponsePlugin
{
    std::optional<std::string> getVersion();

    class TelemetryUtils
    {
    public:
        static void incrementTotalActions(const std::string& type);
        static void incrementFailedActions(const std::string& type);
        static void incrementTimedOutActions(const std::string& type);
        static void incrementExpiredActions(const std::string& type);
        static std::string getRunCommandTelemetry();
    };

    struct actionTelemetry
    {
        int total = 0;
        int totalFailures = 0;
        int timeoutFailures = 0;
        int expiryFailures = 0;
    };
} // namespace ResponsePlugin