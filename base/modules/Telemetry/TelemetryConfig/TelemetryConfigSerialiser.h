/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Config.h"

//#include <json.hpp>
#include <json.hpp>
#include <string>

namespace Telemetry::TelemetryConfig
{
    class TelemetryConfigSerialiser
    {
    public:
        static std::string serialise(const Config& telemetryConfigObject);
        static Config deserialise(const std::string& jsonString);
    };

    void to_json(nlohmann::json& j, const Config& config);
    void from_json(const nlohmann::json& j, Config& config);

    //void to_json(nlohmann::json& j, const TelemetryObject& telemetryObject);
    //void from_json(const nlohmann::json& j, TelemetryObject& telemetryObject);

}
