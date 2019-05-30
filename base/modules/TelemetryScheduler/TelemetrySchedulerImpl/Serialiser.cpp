/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Serialiser.h"

#include <json.hpp>
#include <sstream>

namespace TelemetrySchedulerImpl
{
    void to_json(nlohmann::json& j, const SchedulerConfig& config);
    void from_json(const nlohmann::json& j, SchedulerConfig& config);

    void to_json(nlohmann::json& j, const SchedulerConfig& config)
    {
        j = nlohmann::json{ { "ScheduledTime", config.getTelemetryScheduledTime() } };
    }

    void from_json(const nlohmann::json& j, SchedulerConfig& config)
    {
        config.setTelemetryScheduledTime(j.at("ScheduledTime"));
    }

    std::string Serialiser::serialise(const SchedulerConfig& config)
    {
        if (!config.isValid())
        {
            throw std::invalid_argument("Configuration input is invalid and cannot be serialised");
        }

        nlohmann::json j = config;

        return j.dump();
    }

    SchedulerConfig Serialiser::deserialise(const std::string& jsonString)
    {
        SchedulerConfig config;

        try
        {
            nlohmann::json j = nlohmann::json::parse(jsonString);
            config = j;
        }
        // As well as basic JSON parsing errors, building config object can also fail, so catch all JSON exceptions.
        catch (nlohmann::detail::exception& e)
        {
            std::stringstream msg;
            msg << "Configuration JSON is invalid: " << e.what();
            throw std::runtime_error(msg.str());
        }

        if (!config.isValid())
        {
            throw std::runtime_error("Configuration from deserialised JSON is invalid");
        }

        return config;
    }
} // namespace TelemetrySchedulerImpl