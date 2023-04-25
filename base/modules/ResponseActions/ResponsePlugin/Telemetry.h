// Copyright 2023 Sophos Limited. All rights reserved.


#pragma once

#include <map>
#include <optional>
#include <string>

namespace ResponsePlugin
{
    std::optional<std::string> getVersion();

    class TelemetryUtils
    {

    public:

        struct actionTelemetry
        {
            long int total = 0;
            long int totalFailures = 0;
            long int timeoutFailures = 0;
            long int expiryFailures = 0;
        };

        static void incrementTotalActions(const std::string& type);
        static void incrementFailedActions(const std::string& type);
        static void incrementTimedOutActions(const std::string& type);
        static void incrementExpiredActions(const std::string& type);
        static TelemetryUtils::actionTelemetry getRunCommandTelemetry();
    };

} // namespace ResponsePlugin