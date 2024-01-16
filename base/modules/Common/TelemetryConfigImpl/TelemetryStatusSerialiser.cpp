// Copyright 2024 Sophos Limited. All rights reserved.

#include "TelemetryStatusSerialiser.h"

#include "Common/Exceptions/IException.h"
#include "Common/TelemetryConfigImpl/Serialiser.h"

#include <climits>
#include <nlohmann/json.hpp>
#include <sstream>

namespace Common::TelemetryConfigImpl
{
    const std::string TELEMETRY_RUN_TIME_KEY = "start-time";

    void to_json(nlohmann::json& j, const TelemetryStatus& config)
    {
        j = nlohmann::json{ { TELEMETRY_RUN_TIME_KEY, config.getTelemetryStartTimeInSecondsSinceEpoch() } };
    }

    void from_json(const nlohmann::json& j, TelemetryStatus& config)
    {
        auto value = j.at(TELEMETRY_RUN_TIME_KEY);
        if (!value.is_number_unsigned() || value.get<unsigned long>() > (unsigned long)(INT_MAX))
        {
            throw Common::Exceptions::IException(LOCATION,
                                                 "Value for start time is negative or too large");
        }
        config.setTelemetryStartTimeInSecondsSinceEpoch(value);
    }

    std::string TelemetryStatusSerialiser::serialise(const TelemetryStatus& config)
    {
        if (!config.isValid())
        {
            throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
        }

        nlohmann::json j = config;

        return j.dump();
    }

    TelemetryStatus TelemetryStatusSerialiser::deserialise(const std::string& jsonString)
    {
        TelemetryStatus config;

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
} // namespace TelemetryConfigImpl