// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SchedulerStatusSerialiser.h"

#include "Common/Exceptions/IException.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"

#include <chrono>
#include <climits>
#include <nlohmann/json.hpp>
#include <sstream>

namespace TelemetrySchedulerImpl
{
    const std::string TELEMETRY_SCHEDULED_TIME_KEY = "scheduled-time";
    const std::string TELEMETRY_LAST_START_TIME_KEY = "last-start-time";

    void to_json(nlohmann::json& j, const SchedulerStatus& config);
    void from_json(const nlohmann::json& j, SchedulerStatus& config);

    static time_t toEpochSeconds(const SchedulerStatus::time_point& time)
    {
        return std::chrono::system_clock::to_time_t(time);
    }

    static SchedulerStatus::time_point fromEpochSeconds(const time_t time)
    {
        return std::chrono::system_clock::from_time_t(time);
    }

    void to_json(nlohmann::json& j, const SchedulerStatus& config)
    {
        j = nlohmann::json{
            { TELEMETRY_SCHEDULED_TIME_KEY, toEpochSeconds(config.getTelemetryScheduledTime()) },
        };
        auto lastTime = config.getLastTelemetryStartTime();
        if (lastTime)
        {
            j[TELEMETRY_LAST_START_TIME_KEY] = toEpochSeconds(lastTime.value());
        }
    }

    void from_json(const nlohmann::json& j, SchedulerStatus& config)
    {
        auto value = j.at(TELEMETRY_SCHEDULED_TIME_KEY);
        if (!value.is_number_unsigned() || value.get<unsigned long>() > (unsigned long)(INT_MAX))
        {
            throw Common::Exceptions::IException(LOCATION,
                "Value for scheduled time is negative or too large");
        }
        config.setTelemetryScheduledTime(fromEpochSeconds(value));

        try {
            value = j.at(TELEMETRY_LAST_START_TIME_KEY);
            if (!value.is_number_unsigned()) {
                throw Common::Exceptions::IException(LOCATION,
                                                     "Value for start time is negative");
            }
            config.setLastTelemetryStartTime(fromEpochSeconds(value));
        }
        catch (const nlohmann::json::out_of_range&)
        {
            // Ignore missing key
        }
    }

    std::string SchedulerStatusSerialiser::serialise(const SchedulerStatus& config)
    {
        if (!config.isValid())
        {
            throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
        }

        nlohmann::json j = config;

        return j.dump();
    }

    SchedulerStatus SchedulerStatusSerialiser::deserialise(const std::string& jsonString)
    {
        SchedulerStatus config;

        try
        {
            nlohmann::json j = nlohmann::json::parse(jsonString);
            config = j;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (const nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            msg << "Configuration JSON is invalid: " << e.what();
            std::throw_with_nested(Common::Exceptions::IException(LOCATION, msg.str()));
        }

        if (!config.isValid())
        {
            throw Common::Exceptions::IException(LOCATION, "Configuration from deserialised JSON is invalid");
        }

        return config;
    }
} // namespace TelemetrySchedulerImpl