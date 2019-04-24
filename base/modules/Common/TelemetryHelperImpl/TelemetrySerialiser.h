/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "TelemetryObject.h"
#include "TelemetryValue.h"

#include <json.hpp>
#include <string>

namespace Common::Telemetry
{
    class TelemetrySerialiser
    {
    public:
        static std::string serialise(const Common::Telemetry::TelemetryObject& telemetryObject);
        static Common::Telemetry::TelemetryObject deserialise(const std::string& jsonString);
    };

    void to_json(nlohmann::json& j, const TelemetryValue& node);
    void from_json(const nlohmann::json& j, TelemetryValue& node);

    void to_json(nlohmann::json& j, const TelemetryObject& telemetryObject);
    void from_json(const nlohmann::json& j, TelemetryObject& telemetryObject);
} // namespace Common::Telemetry
