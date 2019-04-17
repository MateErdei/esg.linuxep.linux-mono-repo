/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <string>
#include "TelemetryNode.h"
#include "TelemetryValue.h"
#include "TelemetryDictionary.h"

namespace Common::Telemetry
{
    class TelemetrySerialiser
    {
    public:
        std::string serialise(const Common::Telemetry::TelemetryNode& node);
    };

    void to_json(nlohmann::json& j, const TelemetryValue& node);
    void from_json(const nlohmann::json& j, TelemetryValue& node);

    void to_json(nlohmann::json& j, const TelemetryDictionary& node);
    void from_json(const nlohmann::json& j, TelemetryDictionary& node);
}
