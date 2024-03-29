/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "TelemetryObject.h"
#include "TelemetryValue.h"

#include <string>

namespace Common::Telemetry
{
    class TelemetrySerialiser
    {
    public:
        static std::string serialise(const Common::Telemetry::TelemetryObject& telemetryObject);
        static Common::Telemetry::TelemetryObject deserialise(const std::string& jsonString);
    };

} // namespace Common::Telemetry
