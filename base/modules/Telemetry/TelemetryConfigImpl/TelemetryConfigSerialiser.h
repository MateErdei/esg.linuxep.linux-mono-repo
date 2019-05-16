/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Config.h"

#include <json.hpp>
#include <string>

namespace Telemetry::TelemetryConfigImpl
{
    class TelemetryConfigSerialiser
    {
    public:
        static std::string serialise(const Config& config);
        static Config deserialise(const std::string& jsonString);
    };

    void to_json(nlohmann::json& j, const Config& config);
    void from_json(const nlohmann::json& j, Config& config);

    void to_json(nlohmann::json& j, const Proxy& proxy);
    void from_json(const nlohmann::json& j, Proxy& proxy);

    void to_json(nlohmann::json& j, const MessageRelay& messageRelay);
    void from_json(const nlohmann::json& j, MessageRelay& messageRelay);
} // namespace Telemetry::TelemetryConfig
